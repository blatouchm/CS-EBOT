//
// Copyright (c) 2003-2009, by Yet Another POD-Bot Development Team.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// $Id:$
//

#include "../include/core.h"
constexpr int16_t pMax = static_cast<int16_t>(Const_MaxPathIndex);

ConVar ebot_zombies_as_path_cost("ebot_zombie_count_as_path_cost", "1");
ConVar ebot_has_semiclip("ebot_has_semiclip", "0");
ConVar ebot_breakable_health_limit("ebot_breakable_health_limit", "3000.0");
ConVar ebot_touch_breakable_classnames("ebot_touch_breakable_classnames", "");
ConVar ebot_force_shortest_path("ebot_force_shortest_path", "0");
ConVar ebot_pathfinder_seed_min("ebot_pathfinder_seed_min", "0.9");
ConVar ebot_pathfinder_seed_max("ebot_pathfinder_seed_max", "1.1");
ConVar ebot_helicopter_width("ebot_helicopter_width", "54.0");
ConVar ebot_use_pathfinding_for_avoid("ebot_use_pathfinding_for_avoid", "1");
ConVar ebot_leap_zombies("ebot_leap_zombies", "0");

extern ConVar ebot_analyze_max_jump_height;
extern ConVar ebot_human_double_jump;
extern ConVar ebot_zombie_double_jump;
extern ConVar ebot_debug;

static inline bool IsDoubleJumpEnabledForBot(const Bot* bot) {
  if (!bot)
    return false;

  return bot->m_isZombieBot ? ebot_zombie_double_jump.GetBool()
                            : ebot_human_double_jump.GetBool();
}

static inline float GetBaseJumpHeightForBot(const Bot* bot) {
  if (!bot || !bot->pev)
    return 0.0f;

  const float gravity = bot->pev->gravity > 0.0f ? bot->pev->gravity : 1.0f;
  return ebot_analyze_max_jump_height.GetFloat() * gravity;
}

static inline bool ShouldUseDoubleJumpForGoal(const Bot* bot, const Vector& goal) {
  if (!bot || !bot->pev || !IsDoubleJumpEnabledForBot(bot))
    return false;

  const float baseJumpHeight = GetBaseJumpHeightForBot(bot);
  if (baseJumpHeight <= 0.0f)
    return false;

  const float goalHeight = goal.z - bot->pev->origin.z;
  return goalHeight > baseJumpHeight && goalHeight < (baseJumpHeight * 2.0f);
}

static inline void TryStartDoubleJump(Bot* bot, const Vector& goal) {
  if (!bot || bot->m_doubleJumpPending)
    return;

  if (!ShouldUseDoubleJumpForGoal(bot, goal))
    return;

  bot->m_doubleJumpPending = true;
  bot->m_doubleJumpTime = engine->GetTime() + 0.3f;
}

static inline bool ProcessDoubleJump(Bot* bot) {
  if (!bot || !bot->m_doubleJumpPending || !bot->pev)
    return false;

  const float time = engine->GetTime();
  if (time < bot->m_doubleJumpTime)
    return false;

  const bool onFloor = !!(bot->pev->flags & (FL_ONGROUND | FL_PARTIALGROUND));
  const bool onLadder = bot->pev->movetype == MOVETYPE_FLY;
  const bool inWater = bot->pev->waterlevel > 2;

  if (!onFloor && !onLadder && !inWater) {
    bot->m_doubleJumpPending = false;
    bot->m_doubleJumpTime = 0.0f;
    return true;
  }

  if (time > bot->m_doubleJumpTime + 0.5f) {
    bot->m_doubleJumpPending = false;
    bot->m_doubleJumpTime = 0.0f;
  }

  return false;
}

struct TouchBreakableClassCache {
  char m_raw[256]{0};
  char m_items[32][64]{};
  int m_count{0};

  void RefreshIfNeeded(const char *csv) {
    if (IsNullString(csv))
      csv = "";

    if (!cstrcmp(m_raw, csv))
      return;

    cstrncpy(m_raw, csv, sizeof(m_raw));
    m_raw[sizeof(m_raw) - 1] = '\0';
    m_count = 0;

    char token[64];
    int tokenLen = 0;
    for (int i = 0;; i++) {
      const char ch = m_raw[i];
      if (ch == ',' || ch == '\0') {
        token[tokenLen] = '\0';
        cstrtrim(token);

        if (!IsNullString(token) &&
            m_count < static_cast<int>(sizeof(m_items) / sizeof(m_items[0]))) {
          cstrncpy(m_items[m_count], token, sizeof(m_items[m_count]));
          m_items[m_count][sizeof(m_items[m_count]) - 1] = '\0';
          m_count++;
        }

        tokenLen = 0;
        if (ch == '\0')
          break;
        continue;
      }

      if (tokenLen < static_cast<int>(sizeof(token)) - 1)
        token[tokenLen++] = ch;
    }
  }

  bool Contains(const char *className) const {
    if (IsNullString(className))
      return false;

    for (int i = 0; i < m_count; i++) {
      if (!cstricmp(m_items[i], className))
        return true;
    }

    return false;
  }
};

static TouchBreakableClassCache s_touchBreakableClassCache;

static inline bool IsExtraTouchBreakableClass(const char *className) {
  s_touchBreakableClassCache.RefreshIfNeeded(
      ebot_touch_breakable_classnames.GetString());
  return s_touchBreakableClassCache.Contains(className);
}

int16_t Bot::FindGoalZombie(void) {
  if (g_waypoint->m_terrorPoints.IsEmpty()) {
    m_currentGoalIndex =
        static_cast<int16_t>(crandomint(0, g_numWaypoints - 1));
    return m_currentGoalIndex;
  }

  if (crandomint(1, 3) == 1) {
    m_currentGoalIndex =
        static_cast<int16_t>(crandomint(0, g_numWaypoints - 1));
    return m_currentGoalIndex;
  }

  m_currentGoalIndex = g_waypoint->m_terrorPoints.Random();
  return m_currentGoalIndex;
}

inline float GetWaypointDistance(const int16_t &start, const int16_t &goal) {
  if (g_isMatrixReady)
    return static_cast<float>(
        *(g_waypoint->m_distMatrix.Get() + (start * g_numWaypoints) + goal));

  return GetVectorDistanceSSE(g_waypoint->m_paths[start].origin,
                              g_waypoint->m_paths[goal].origin);
}

static inline bool IsFlaglessWaypoint(const int16_t index) {
  if (!IsValidWaypoint(index))
    return false;

  return g_waypoint->m_paths[index].flags == 0;
}

int16_t Bot::FindGoalHuman(void) {
  if (!IsValidWaypoint(m_currentWaypointIndex))
    FindWaypoint();

  if (!IsValidWaypoint(m_currentWaypointIndex)) {
    if (g_numWaypoints > 0)
      m_currentGoalIndex = static_cast<int16_t>(crandomint(0, g_numWaypoints - 1));
    else
      m_currentGoalIndex = -1;
    return m_currentGoalIndex;
  }

  if (m_skipHumanCampThisRound) {
    int16_t candidate = -1;
    for (int attempt = 0; attempt < 64; attempt++) {
      const int16_t randomIndex =
          static_cast<int16_t>(crandomint(0, g_numWaypoints - 1));
      if (IsFlaglessWaypoint(randomIndex)) {
        candidate = randomIndex;
        break;
      }
    }

    if (!IsValidWaypoint(candidate)) {
      for (int16_t i = 0; i < g_numWaypoints; i++) {
        if (IsFlaglessWaypoint(i)) {
          candidate = i;
          break;
        }
      }
    }

    if (!IsValidWaypoint(candidate))
      candidate = static_cast<int16_t>(crandomint(0, g_numWaypoints - 1));

    m_currentGoalIndex = candidate;
    return m_currentGoalIndex;
  }

  if (IsValidWaypoint(m_myMeshWaypoint)) {
    m_currentGoalIndex = m_myMeshWaypoint;
    return m_currentGoalIndex;
  } else if (IsValidWaypoint(m_zhCampPointIndex)) {
    m_currentGoalIndex = m_zhCampPointIndex;
    return m_currentGoalIndex;
  } else {
    if (!g_waypoint->m_zmHmPoints.IsEmpty()) {
      if (g_DelayTimer < engine->GetTime()) {
        float dist, maxDist = 999999999999.0f;
        int16_t i, index = -1, temp;

        if (m_numFriendsLeft) {
          for (i = 0; i < g_waypoint->m_zmHmPoints.Size(); i++) {
            temp = g_waypoint->m_zmHmPoints.Get(i);
            dist = GetWaypointDistance(m_currentWaypointIndex, temp);
            if (dist > maxDist)
              continue;

            // bots won't left you alone like other persons in your life...
            if (!IsFriendReachableToPosition(g_waypoint->m_paths[temp].origin))
              continue;

            index = temp;
            maxDist = dist;
          }

          if (!IsValidWaypoint(index)) {
            maxDist = 999999999999.0f;
            int16_t i, index = -1, temp;
            for (i = 0; i < g_waypoint->m_zmHmPoints.Size(); i++) {
              temp = g_waypoint->m_zmHmPoints.Get(i);
              dist = GetWaypointDistance(m_currentWaypointIndex, temp);
              if (dist > maxDist)
                continue;

              // no friends nearby? go to safe camp spot
              if (IsEnemyReachableToPosition(g_waypoint->m_paths[temp].origin))
                continue;

              index = temp;
              maxDist = dist;
            }
          }
        }

        // if we are alone just stay to nearest
        // theres nothing we can do...
        if (!IsValidWaypoint(index)) {
          maxDist = 999999999999.0f;
          for (i = 0; i < g_waypoint->m_zmHmPoints.Size(); i++) {
            temp = g_waypoint->m_zmHmPoints.Get(i);
            dist = GetWaypointDistance(m_currentWaypointIndex, temp);

            // at least get nearest camp spot
            if (dist > maxDist)
              continue;

            index = temp;
            maxDist = dist;
          }
        }

        if (IsValidWaypoint(index)) {
          m_currentGoalIndex = index;
          return m_currentGoalIndex;
        } else {
          m_currentGoalIndex = g_waypoint->m_zmHmPoints.Random();
          return m_currentGoalIndex;
        }
      } else {
        m_currentGoalIndex = g_waypoint->m_zmHmPoints.Random();
        return m_currentGoalIndex;
      }
    }
  }

  m_currentGoalIndex = static_cast<int16_t>(crandomint(0, g_numWaypoints - 1));
  return m_currentGoalIndex;
}

void Bot::MoveTo(const Vector &targetPosition, const bool checkStuck) {
  const Vector directionOld =
      (targetPosition + pev->velocity * -m_frameInterval) -
      (pev->origin + pev->velocity * m_frameInterval);
  m_moveAngles = directionOld.ToAngles();
  m_moveAngles.x = -m_moveAngles.x; // invert for engine
  m_moveAngles.ClampAngles();
  m_moveSpeed = pev->maxspeed;

  if (checkStuck)
    CheckStuck(directionOld.Normalize2D(), m_frameInterval);
}

void Bot::MoveOut(const Vector &targetPosition, const bool checkStuck) {
  const Vector directionOld = targetPosition - pev->origin;
  SetStrafeSpeed(directionOld.Normalize2D(), pev->maxspeed);
  m_moveAngles = directionOld.ToAngles();
  m_moveAngles.x = -m_moveAngles.x; // invert for engine
  m_moveAngles.ClampAngles();
  m_moveSpeed = -pev->maxspeed;

  if (checkStuck)
    CheckStuck((pev->origin - targetPosition).Normalize2D(), m_frameInterval);
}

void Bot::FollowPath(void) { m_navNode.Start(); }

// this function is a main path navigation
void Bot::DoWaypointNav(void) {
  m_destOrigin = m_waypointOrigin;

  // Guard against invalid current waypoint index before any direct
  // g_waypoint->m_paths[m_currentWaypointIndex] access below.
  if (!IsValidWaypoint(m_currentWaypointIndex)) {
    FindWaypoint();

    if (!IsValidWaypoint(m_currentWaypointIndex) &&
        IsValidWaypoint(m_navNode.First()))
      ChangeWptIndex(m_navNode.First());

    if (!IsValidWaypoint(m_currentWaypointIndex)) {
      m_navNode.Clear();
      m_moveSpeed = 0.0f;
      m_strafeSpeed = 0.0f;
      m_buttons &=
          ~(IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT | IN_JUMP);
      return;
    }
    m_destOrigin = m_waypointOrigin;
  }

  bool ladderJumpLink = false;
  int16_t ladderJumpTargetIndex = -1;
  Vector ladderJumpTarget = m_destOrigin;
  Vector ladderJumpAimTarget = m_destOrigin;
  if (IsOnLadder() && m_navNode.HasNext()) {
    const int16_t nextIndex = m_navNode.Next();
    for (int16_t i = 0; i < pMax; i++) {
      if (m_waypoint.index[i] == nextIndex) {
        ladderJumpLink = (m_waypoint.connectionFlags[i] & PATHFLAG_JUMP) != 0;
        ladderJumpTargetIndex = nextIndex;
        const Path *nextPath = g_waypoint->GetPath(nextIndex);
        if (nextPath)
          ladderJumpTarget = nextPath->origin;
        break;
      }
    }
  }
  if (ladderJumpLink)
    ladderJumpAimTarget = ladderJumpTarget;

  const bool ladderJumpVisible =
      IsOnLadder() && ladderJumpLink && IsInViewCone(ladderJumpAimTarget);
  const bool ladderJumpAimReady =
      ladderJumpVisible &&
      InFieldOfView(ladderJumpAimTarget - EyePosition()) <= 10.0f;
  const float currentTime = engine->GetTime();
  const bool debugJump = ebot_debug.GetBool();
  auto getGroundJumpLookIndex = [&]() -> int16_t {
    int16_t jumpLookIndex = m_navNode.HasNext() ? m_navNode.Next() : -1;
    if (!IsValidWaypoint(jumpLookIndex) && IsValidWaypoint(m_currentWaypointIndex))
      jumpLookIndex = m_currentWaypointIndex;
    if (!IsValidWaypoint(jumpLookIndex) && IsValidWaypoint(m_navNode.First()))
      jumpLookIndex = m_navNode.First();
    return jumpLookIndex;
  };

  if (m_jumpLookTargetActive && m_jumpLookDeadline > 0.0f &&
      currentTime >= m_jumpLookDeadline) {
    m_jumpLookTargetActive = false;
    m_jumpLookTarget = -1;
    m_jumpLookDeadline = 0.0f;
  }

  const bool currentIsLadderWaypoint =
      IsValidWaypoint(m_currentWaypointIndex) && (m_waypoint.flags & WAYPOINT_LADDER);
  // Keep timeout active even if bot keeps hopping under the ladder.
  if (currentIsLadderWaypoint && !IsOnLadder()) {
    if (m_ladderGroundStartTime <= 0.0f)
      m_ladderGroundStartTime = currentTime;
    else if (currentTime - m_ladderGroundStartTime >= 2.0f) {
      m_ladderGroundStartTime = 0.0f;
      m_ladderJumpPrepTime = 0.0f;
      m_ladderJumpRetryDeadline = 0.0f;
      m_ladderJumpRetrySource = -1;
      m_ladderJumpRetryTarget = -1;
      m_ladderJumpInitialPressUsed = false;
      m_waitForLanding = false;
      m_navNode.Clear();
      ChangeWptIndex(-1);
      FindWaypoint();
      return;
    }
  } else
    m_ladderGroundStartTime = 0.0f;

  auto resetLadderJumpRetry = [&]() {
    const int16_t retrySource = m_ladderJumpRetrySource;
    const int16_t retryTarget = m_ladderJumpRetryTarget;
    m_ladderJumpPrepTime = 0.0f;
    m_ladderJumpRetryDeadline = 0.0f;
    m_ladderJumpRetrySource = -1;
    m_ladderJumpRetryTarget = -1;
    m_ladderJumpInitialPressUsed = false;
    m_waitForLanding = false;
    m_jumpReady = false;
    m_doubleJumpPending = false;
    m_doubleJumpTime = 0.0f;
    m_buttons &= ~(IN_JUMP | IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT);
    m_moveSpeed = 0.0f;
    m_strafeSpeed = 0.0f;

    m_navNode.Start();
    if (m_navNode.CanFollowPath() && IsValidWaypoint(retrySource)) {
      m_navNode.Set(0, retrySource);
      if (IsValidWaypoint(retryTarget))
        m_navNode.Set(1, retryTarget);
    }

    if (IsValidWaypoint(retrySource))
      ChangeWptIndex(retrySource);
    else if (m_navNode.CanFollowPath() && IsValidWaypoint(m_navNode.First()))
      ChangeWptIndex(m_navNode.First());
  };

  if (IsValidWaypoint(m_ladderJumpRetryTarget) &&
      m_ladderJumpRetryDeadline > 0.0f) {
    const Path *retryTargetPath = g_waypoint->GetPath(m_ladderJumpRetryTarget);
    if (!retryTargetPath) {
      if (debugJump) {
        ServerPrint("%s ladder jump lock end: target waypoint became invalid (src %d, dst %d)",
                    GetEntityName(m_myself), m_ladderJumpRetrySource,
                    m_ladderJumpRetryTarget);
      }
      m_ladderJumpPrepTime = 0.0f;
      m_ladderJumpRetryDeadline = 0.0f;
      m_ladderJumpRetrySource = -1;
      m_ladderJumpRetryTarget = -1;
      m_ladderJumpInitialPressUsed = false;
      m_waitForLanding = false;
    } else {
      const float distToRetryTargetSq =
          (pev->origin - retryTargetPath->origin).GetLengthSquared();
      const float distToRetryTarget = csqrtf(distToRetryTargetSq);
      const bool reachedRetryTargetByDistance =
          distToRetryTargetSq <= squaredf(40.0f);
      bool fartherFromSourceThanTarget = true;
      if (const Path *const retrySourcePath =
              g_waypoint->GetPath(m_ladderJumpRetrySource)) {
        const float distToRetrySourceSq =
            (pev->origin - retrySourcePath->origin).GetLengthSquared();
        fartherFromSourceThanTarget = distToRetrySourceSq > distToRetryTargetSq;
      }
      const bool targetIsLadder = (retryTargetPath->flags & WAYPOINT_LADDER) != 0;
      const bool stableAfterJump =
          targetIsLadder ? IsOnLadder() : (IsOnFloor() || IsInWater());

      if (reachedRetryTargetByDistance && fartherFromSourceThanTarget &&
          stableAfterJump) {
        const int16_t reachedTargetIndex = m_ladderJumpRetryTarget;
        if (debugJump) {
          ServerPrint("%s ladder jump lock end: reached target (dist %.1f)",
                      GetEntityName(m_myself), distToRetryTarget);
        }
        m_ladderJumpPrepTime = 0.0f;
        m_ladderJumpRetryDeadline = 0.0f;
        m_ladderJumpRetrySource = -1;
        m_ladderJumpRetryTarget = -1;
        m_ladderJumpInitialPressUsed = false;
        m_waitForLanding = false;
        if (IsValidWaypoint(reachedTargetIndex))
          ChangeWptIndex(reachedTargetIndex);
      } else if (IsOnFloor() || IsInWater()) {
        // Abort ladder jump process after landing.
        if (debugJump) {
          ServerPrint("%s ladder jump lock end: landed before target (dist %.1f)",
                      GetEntityName(m_myself), distToRetryTarget);
        }
        m_ladderJumpPrepTime = 0.0f;
        m_ladderJumpRetryDeadline = 0.0f;
        m_ladderJumpRetrySource = -1;
        m_ladderJumpRetryTarget = -1;
        m_ladderJumpInitialPressUsed = false;
        m_waitForLanding = false;
      } else if (currentTime >= m_ladderJumpRetryDeadline) {
        if (debugJump) {
          ServerPrint("%s ladder jump lock end: timeout (dist %.1f)",
                      GetEntityName(m_myself), distToRetryTarget);
        }
        resetLadderJumpRetry();
        return;
      } else {
        // While ladder jump is in flight, keep view/buttons locked.
        m_waitForLanding = true;
        m_updateLooking = true;
        m_lookVelocity = nullvec;
        m_lookAt = retryTargetPath->origin;
        m_buttons &= ~(IN_BACK | IN_MOVELEFT | IN_MOVERIGHT | IN_DUCK);
        m_buttons |= IN_FORWARD;
        if (!m_ladderJumpInitialPressUsed) {
          m_buttons |= IN_JUMP;
          m_ladderJumpInitialPressUsed = true;
        } else
          m_buttons &= ~IN_JUMP;
        return;
      }
    }
  } else {
    m_ladderJumpRetryDeadline = 0.0f;
    m_ladderJumpRetrySource = -1;
    m_ladderJumpRetryTarget = -1;
    m_ladderJumpInitialPressUsed = false;
  }

  const bool ladderJumpWindow =
      IsOnLadder() && ladderJumpLink &&
      (pev->origin - m_waypoint.origin).GetLengthSquared() < squaredf(72.0f);

  if (IsOnLadder()) {
    // Keep ladder movement deterministic; allow jump only on explicit jump links.
    m_doubleJumpPending = false;
    m_doubleJumpTime = 0.0f;
    m_jumpReady = false;

    if (ladderJumpLink) {
      // If we are not yet at jump source point, return to it first.
      if (!ladderJumpWindow && !m_waitForLanding) {
        m_ladderJumpPrepTime = 0.0f;
        m_buttons &= ~IN_JUMP;
        m_updateLooking = true;
        m_lookVelocity = nullvec;
        m_lookAt = m_waypoint.origin;
      } else {
      // Keep looking at jump target so the view-cone condition can become true.
      m_updateLooking = true;
      m_lookVelocity = nullvec;
      m_lookAt = ladderJumpAimTarget;

      if (!ladderJumpAimReady) {
        m_ladderJumpPrepTime = 0.0f;
        m_waitForLanding = false;
        m_buttons &= ~IN_JUMP;
      } else if (ladderJumpWindow && !m_waitForLanding) {
        if (m_ladderJumpPrepTime <= 0.0f)
          m_ladderJumpPrepTime = currentTime + 0.5f;

        // Before ladder jump, stop and lock aim for 0.5 second.
        if (currentTime < m_ladderJumpPrepTime) {
          m_moveSpeed = 0.0f;
          m_strafeSpeed = 0.0f;
          m_buttons &=
              ~(IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT | IN_JUMP);
        } else {
          const int16_t jumpSourceIndex = m_currentWaypointIndex;

          m_duckTime = engine->GetTime() + 1.25f;
          m_buttons |= (IN_FORWARD | IN_JUMP);


          if (debugJump) {
            ServerPrint("%s ladder jump: %d -> %d vel(%.1f %.1f %.1f)",
                        GetEntityName(m_myself), m_currentWaypointIndex,
                        ladderJumpTargetIndex, pev->velocity.x, pev->velocity.y,
                        pev->velocity.z);
          }

          m_waitForLanding = true;
          m_ladderJumpRetryDeadline = currentTime + 2.0f;
          m_ladderJumpRetrySource = jumpSourceIndex;
          m_ladderJumpRetryTarget = ladderJumpTargetIndex;
          m_ladderJumpInitialPressUsed = false;
          m_ladderJumpPrepTime = 0.0f;
        }
      } else if (!m_waitForLanding) {
        m_ladderJumpPrepTime = 0.0f;
      }

      if (!m_waitForLanding && m_ladderJumpPrepTime > currentTime)
      {
        m_buttons &= ~IN_JUMP;
      }
      else if (ladderJumpVisible && ladderJumpWindow)
      {
        m_duckTime = engine->GetTime() + 1.25f;
        m_buttons |= (IN_FORWARD | IN_JUMP);
      }
      else
        m_buttons &= ~IN_JUMP;
      }
    } else {
      m_ladderJumpPrepTime = 0.0f;
      m_waitForLanding = false;
      m_buttons &= ~IN_JUMP;
    }
  }

  if (!IsOnLadder() && (m_currentTravelFlags & PATHFLAG_JUMP) &&
      IsValidWaypoint(m_currentWaypointIndex)) {
    // Keep aim locked on jump destination for every non-ladder jump link.
    m_updateLooking = true;
    m_lookVelocity = nullvec;
    const int16_t jumpLookIndex = getGroundJumpLookIndex();
    if (IsValidWaypoint(jumpLookIndex)) {
      if (const Path *const jumpLookPath = g_waypoint->GetPath(jumpLookIndex))
        m_lookAt = jumpLookPath->origin;
      else
        m_lookAt = m_waypoint.origin;
    } else
      m_lookAt = m_waypoint.origin;
  }

  if (!IsOnLadder() && ProcessDoubleJump(this))
    m_buttons |= IN_JUMP;

  if (m_jumpReady && !m_waitForLanding) {
    if (IsOnLadder()) {
      m_buttons &= ~IN_JUMP;
      m_jumpReady = false;
      m_waitForLanding = false;
      return;
    }

    if (IsOnFloor()) {
      const Vector myOrigin = GetBottomOrigin(m_myself);
      Vector waypointOrigin =
          m_waypoint.origin; // directly jump to waypoint, ignore risk of fall

      if (m_waypoint.flags & WAYPOINT_CROUCH)
        waypointOrigin.z -= 18.0f;
      else
        waypointOrigin.z -= 36.0f;

      if (ebot_debug.GetBool()) {
        m_debugJumpTarget = waypointOrigin;
        m_debugJumpTargetTime = engine->GetTime() + 3.0f;
      }

      Vector sourceWaypointOrigin = myOrigin;
      if (IsValidWaypoint(m_prevWptIndex[1])) {
        if (const Path *const sourceWaypoint = g_waypoint->GetPath(m_prevWptIndex[1]))
          sourceWaypointOrigin = sourceWaypoint->origin;
      }

      const float sourceWaypointZ = cminf(sourceWaypointOrigin.z, myOrigin.z);
      const bool jumpingToHigherWaypoint = (m_waypoint.origin.z - sourceWaypointZ) > 1.0f;
      const bool shortUpwardJump =
          jumpingToHigherWaypoint &&
          (m_waypoint.origin - sourceWaypointOrigin).GetLengthSquared() <
              squaredf(50.0f);

      Vector velocity = CheckThrow(myOrigin, waypointOrigin);
      if (velocity.GetLengthSquared() < squaredf(100.0f))
        velocity = CheckToss(myOrigin, waypointOrigin);
      velocity = velocity + velocity * 0.45f;

      // Keep jump arc closer to player-like height across custom gravity values.
      float gravityFactor = pev->gravity;
      if (Math::FltZero(gravityFactor) || gravityFactor < 0.0f)
          gravityFactor = 1.0f;

      const float gravityScale = csqrtf(gravityFactor);

      //There are three types of jumps from the ground: 
      // a calculated long jump (boosted velocity) 
      // a short vertical jump (without xy, with IN_DUCK + IN_FORWARD)
      // other jumps (if vertical -> IN_DUCK + IN_FORWARD)

      // set the bot "grenade" velocity - a calculated long jump (boosted velocity) 
      if (velocity.GetLengthSquared() > 10.0f && !shortUpwardJump) {

        const bool longUpwardJump =
              jumpingToHigherWaypoint &&
              (m_waypoint.origin - sourceWaypointOrigin).GetLengthSquared() >
              squaredf(100.0f);

        const float jumpVelocityXYBoost = 1.0f;
        float jumpVelocityZBoost = 1.2f;

        pev->velocity.x = velocity.x * jumpVelocityXYBoost;
        pev->velocity.y = velocity.y * jumpVelocityXYBoost;

        //needed for long jump to higher ladder etc.
        if(longUpwardJump)
            jumpVelocityZBoost = 1.5f;

        float jumpVelocityZ = velocity.z * gravityScale * 1.15f * jumpVelocityZBoost;
        const float minJumpVelocityZ = 260.0f * gravityScale * jumpVelocityZBoost;
        if (jumpVelocityZ < minJumpVelocityZ)
          jumpVelocityZ = minJumpVelocityZ;

        pev->velocity.z = jumpVelocityZ;

        if (debugJump) {
            ServerPrint("%s ground jump - long: %d -> %d vel(%.1f %.1f %.1f)",
                GetEntityName(m_myself), m_currentWaypointIndex,
                ladderJumpTargetIndex, pev->velocity.x, pev->velocity.y,
                pev->velocity.z);
        }

      } else {  //short vertical jump (without xy, with IN_DUCK + IN_FORWARD)
        if (shortUpwardJump) {

          pev->velocity.x = 0.0f;
          pev->velocity.y = 0.0f;
          pev->velocity.z = 260.0f * gravityScale;

          if (debugJump) {
              ServerPrint("%s ground jump - shortUpward: %d -> %d vel(%.1f %.1f %.1f)",
                  GetEntityName(m_myself), m_currentWaypointIndex,
                  ladderJumpTargetIndex, pev->velocity.x, pev->velocity.y,
                  pev->velocity.z);
          }

        } else { //other jumps 

          pev->velocity.x = (waypointOrigin.x - myOrigin.x) *
                            (1.0f + (pev->maxspeed / 500.0f) + pev->gravity);
          pev->velocity.y = (waypointOrigin.y - myOrigin.y) *
                            (1.0f + (pev->maxspeed / 500.0f) + pev->gravity);

          pev->velocity.z = 260.0f * gravityScale;

          if (debugJump) {
              ServerPrint("%s ground jump - other: %d -> %d vel(%.1f %.1f %.1f)",
                  GetEntityName(m_myself), m_currentWaypointIndex,
                  ladderJumpTargetIndex, pev->velocity.x, pev->velocity.y,
                  pev->velocity.z);
          }

        }
      }

      const float jumpStartTime = engine->GetTime();
      if (jumpingToHigherWaypoint) {
        m_jumpDuckStartTime = jumpStartTime + 0.1f;
        m_jumpDuckEndTime = jumpStartTime + 1.25f;
      } else {
        m_jumpDuckStartTime = 0.0f;
        m_jumpDuckEndTime = 0.0f;
      }

      m_jumpLookTarget = getGroundJumpLookIndex();
      m_jumpLookTargetActive = IsValidWaypoint(m_jumpLookTarget);
      m_jumpLookDeadline = m_jumpLookTargetActive ? (jumpStartTime + 3.0f) : 0.0f;

      const bool zombieLeap = m_isZombieBot && ebot_leap_zombies.GetBool();
      const bool crouchTarget = (m_waypoint.flags & WAYPOINT_CROUCH) != 0;

      if (zombieLeap) {
        m_duckTime = engine->GetTime() + 1.25f;
        m_buttons |= (IN_DUCK | IN_JUMP);
      } else {
        m_buttons &= ~IN_JUMP;
        if (crouchTarget) {
          m_duckTime = engine->GetTime() + 1.25f;
          m_buttons |= IN_DUCK;
        } else
          m_buttons &= ~IN_DUCK;
      }

      TryStartDoubleJump(this, m_waypoint.origin);
      m_jumpReady = false;
      m_waitForLanding = true;
    }

    return;
  }

  if (m_waitForLanding) {
    auto clearLandingWait = [&]() {
      m_waitForLanding = false;
      m_jumpReady = false;
      m_doubleJumpPending = false;
      m_doubleJumpTime = 0.0f;
      m_jumpLookTargetActive = false;
      m_jumpLookTarget = -1;
      m_jumpLookDeadline = 0.0f;
    };

    if (IsOnLadder()) {
      if (ladderJumpLink) {
        // Keep the jump held while still attached to ladder during
        // ladder-to-ladder transitions.
        if (ladderJumpVisible) {
          m_buttons |= (IN_DUCK | IN_JUMP);
        }
        else
          m_buttons &= ~IN_JUMP;
        return;
      }

      clearLandingWait();
      m_buttons &= ~IN_JUMP;
      return;
    }

    const bool landed = (IsOnFloor() || IsInWater()) && pev->velocity.z <= 5.0f;
    if (landed) {
      if (!SetProcess(Process::Pause, "waiting a bit for next jump", true,
                      engine->GetTime() + crandomfloat(0.1f, 0.35f)))
        clearLandingWait();
    } else {
      // While airborne after jump, keep looking at jump destination.
      m_updateLooking = true;
      m_lookVelocity = nullvec;
      if (m_jumpLookTargetActive) {
        if (const Path *const jumpLookPath = g_waypoint->GetPath(m_jumpLookTarget))
          m_lookAt = jumpLookPath->origin;
        else {
          m_jumpLookTargetActive = false;
          m_jumpLookTarget = -1;
          m_jumpLookDeadline = 0.0f;
        }
      }

      if (!m_jumpLookTargetActive && IsValidWaypoint(m_currentWaypointIndex))
        m_lookAt = m_waypoint.origin;
      else if (!m_jumpLookTargetActive)
        m_lookAt = m_destOrigin;

      // Forward lock should follow the same delayed window as airborne duck.
      if (m_jumpDuckEndTime > currentTime &&
          currentTime >= m_jumpDuckStartTime) {
        m_buttons &= ~(IN_BACK | IN_MOVELEFT | IN_MOVERIGHT);
        m_buttons |= IN_FORWARD;
      }
    }

    return;
  }

  if (!IsValidWaypoint(m_currentWaypointIndex) &&
      IsValidWaypoint(m_navNode.First())) {
    const int16_t firstIndex = m_navNode.First();
    const Path *const firstPath = g_waypoint->GetPath(firstIndex);
    bool allowAdopt = true;
    if (firstPath->flags & WAYPOINT_LADDER) {
      allowAdopt = (pev->origin - firstPath->origin).GetLengthSquared() <=
                   squaredf(24.0f);
    }

    if (allowAdopt)
      ChangeWptIndex(firstIndex);
  }

  if (m_waypoint.flags & WAYPOINT_FALLCHECK) {
    TraceResult tr;
    const Vector origin = g_waypoint->m_paths[m_currentWaypointIndex].origin;
    TraceLine(origin, origin - Vector(0.0f, 0.0f, 60.0f), TraceIgnore::Nothing,
              m_myself, &tr);
    if (tr.flFraction >= 1.0f) {
      m_navNode.Clear();
      ResetStuck();
      m_moveSpeed = 0.0f;
      m_strafeSpeed = 0.0f;
      return;
    }
  } else if (m_waypoint.flags & WAYPOINT_WAITUNTIL) {
    TraceResult tr;
    const Vector origin = g_waypoint->m_paths[m_currentWaypointIndex].origin;
    TraceLine(origin, m_waypoint.origin - Vector(0.0f, 0.0f, 60.0f),
              TraceIgnore::Nothing, m_myself, &tr);
    if (tr.flFraction >= 1.0f) {
      ResetStuck();
      m_moveSpeed = 0.0f;
      m_strafeSpeed = 0.0f;
      return;
    }
  } else if (m_waypoint.flags & WAYPOINT_HELICOPTER) {
    TraceResult tr;
    if (!FNullEnt(g_helicopter)) {
      const Vector center = GetBoxOrigin(g_helicopter);
      if (IsVisible(center, m_myself)) {
        if ((pev->origin - center).GetLengthSquared2D() < squaredf(70)) {
          ResetStuck();
          m_moveSpeed = 0.0f;
          m_strafeSpeed = 0.0f;
          m_isEnemyReachable = false;
        } else {
          MoveTo(center, false);
          if (IsOnFloor() &&
              pev->velocity.GetLength2D() < (pev->maxspeed * 0.5f))
            m_buttons |= IN_JUMP;
        }
      } else {
        MakeVectors(g_helicopter->v.angles);
        const Vector right =
            center + g_pGlobals->v_right * ebot_helicopter_width.GetFloat();
        const Vector left =
            center - g_pGlobals->v_right * ebot_helicopter_width.GetFloat();
        if ((pev->origin - right).GetLengthSquared2D() <
            (pev->origin - left).GetLengthSquared2D()) {
          TraceLine(center, right, TraceIgnore::Nothing, m_myself, &tr);
          if (tr.flFraction > 0.99f) {
            MoveTo(right, false);
            return;
          }
        }

        TraceLine(center, left, TraceIgnore::Nothing, m_myself, &tr);
        if (tr.flFraction > 0.99f) {
          MoveTo(left, false);
          return;
        }

        MoveTo(center, false);
      }
    } else {
      const Vector origin = g_waypoint->m_paths[m_currentWaypointIndex].origin;
      TraceLine(origin + Vector(0.0f, 0.0f, 36.0f),
                origin - Vector(0.0f, 0.0f, 36.0f), TraceIgnore::Nothing,
                m_myself, &tr);
      if (!FNullEnt(tr.pHit))
        g_helicopter = tr.pHit;
      else {
        ResetStuck();
        m_moveSpeed = 0.0f;
        m_strafeSpeed = 0.0f;
      }
    }

    return;
  }

  if (m_waypoint.flags & WAYPOINT_LIFT) {
    if (!UpdateLiftHandling())
      return;

    if (!UpdateLiftStates())
      return;
  }

  if (m_waypoint.flags & WAYPOINT_CROUCH) {
    if (pev->flags & FL_DUCKING) {
      if (IsOnFloor()) {
        // bots sometimes slow down when they are crouching... related to the
        // engine bug?
        const float speed = pev->velocity.GetLength2D();
        if (speed < (pev->maxspeed * 0.5f)) {
          const float speedNextFrame = speed * 1.25f;
          const Vector vel = (m_waypoint.origin - pev->origin).Normalize2D();
          if (!vel.IsNull()) {
            pev->velocity.x = vel.x * speedNextFrame;
            pev->velocity.y = vel.y * speedNextFrame;
          }
        }
      }
    } else
      m_duckTime = engine->GetTime() + 1.25f;
  }

  // check if we are going through a door...
  if (g_hasDoors && FNullEnt(m_liftEntity)) {
    TraceResult tr;
    TraceLine(pev->origin, m_waypoint.origin, TraceIgnore::Monsters, m_myself,
              &tr);
    if (!FNullEnt(tr.pHit) &&
        !cstrncmp(STRING(tr.pHit->v.classname), "func_door", 9)) {
      // if the door is near enough...
      const Vector origin = GetEntityOrigin(tr.pHit);
      if ((pev->origin - origin).GetLengthSquared() < squaredf(54.0f)) {
        ResetStuck(); // don't consider being stuck

        if (!(m_oldButtons & IN_USE) && !(m_buttons & IN_USE)) {
          LookAt(origin);

          // do not use door directrly under xash, or we will get failed assert
          // in gamedll code
          if (g_gameVersion & Game::Xash)
            m_buttons |= IN_USE;
          else
            MDLL_Use(tr.pHit, m_myself); // also 'use' the door randomly
        }
      }

      const float time = engine->GetTime();
      edict_t *button = FindNearestButton(STRING(tr.pHit->v.targetname));
      if (!FNullEnt(button)) {
        m_buttonEntity = button;
        if (SetProcess(Process::UseButton, "trying to use a button", false,
                       time + 10.0f))
          return;
      }

      // if bot hits the door, then it opens, so wait a bit to let it open
      // safely
      if (m_timeDoorOpen < time && pev->velocity.GetLengthSquared2D() <
                                       squaredf(pev->maxspeed * 0.20f)) {
        m_timeDoorOpen = time + 3.0f;

        if (m_tryOpenDoor++ > 2 && !FNullEnt(m_nearestEnemy) &&
            IsWalkableLineClear(EyePosition(), m_nearestEnemy->v.origin)) {
          m_hasEnemiesNear = true;
          m_enemyOrigin = m_nearestEnemy->v.origin;
          m_tryOpenDoor = 0;
        } else {
          if (m_isSlowThink) {
            SetProcess(Process::Pause, "waiting for door open", true,
                       time + 1.25f);
            m_moveSpeed = 0.0f;
            m_strafeSpeed = 0.0f;
            return;
          }

          m_tryOpenDoor = 0;
        }
      }
    }
  }

  bool next = false;

  if (IsOnLadder() || m_waypoint.flags & WAYPOINT_LADDER) {
    // special detection if someone is using the ladder (to prevent to have
    // bots-towers on ladders)
    TraceResult tr;
    bool foundGround = false;
    int16_t previousNode = 0;
    extern ConVar ebot_ignore_enemies;
    for (const auto &client : g_clients) {
      if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) ||
          FNullEnt(client.ent) || (client.ent->v.movetype != MOVETYPE_FLY) ||
          client.index == m_index)
        continue;

      foundGround = false;
      previousNode = 0;

      // more than likely someone is already using our ladder...
      if ((client.ent->v.origin - m_destOrigin).GetLengthSquared() <
          squaredf(48.0f)) {
        if (client.team != m_team && !ebot_ignore_enemies.GetBool()) {
          TraceLine(EyePosition(), client.ent->v.origin, TraceIgnore::Monsters,
                    m_myself, &tr);

          // bot found an enemy on his ladder - he should see him...
          if (!FNullEnt(tr.pHit) && tr.pHit == client.ent) {
            m_hasEnemiesNear = true;
            m_nearestEnemy = client.ent;
            m_enemyOrigin = m_nearestEnemy->v.origin;
            break;
          }
        } else if (client.team == m_team && !ebot_has_semiclip.GetBool()) {
          TraceHull(EyePosition(), m_destOrigin, TraceIgnore::Monsters,
                    (pev->flags & FL_DUCKING) ? head_hull : human_hull,
                    m_myself, &tr);

          // someone is above or below us and is using the ladder already
          if (client.ent->v.movetype == MOVETYPE_FLY &&
              cabsf(pev->origin.z - client.ent->v.origin.z) > 15.0f &&
              !FNullEnt(tr.pHit) && tr.pHit == client.ent) {
            if (IsValidWaypoint(m_prevWptIndex[0])) {
              if (!(g_waypoint->GetPath(m_prevWptIndex[0])->flags &
                    WAYPOINT_LADDER)) {
                foundGround = true;
                previousNode = m_prevWptIndex[0];
              } else if (IsValidWaypoint(m_prevWptIndex[1])) {
                if (!(g_waypoint->GetPath(m_prevWptIndex[1])->flags &
                      WAYPOINT_LADDER)) {
                  foundGround = true;
                  previousNode = m_prevWptIndex[1];
                } else if (IsValidWaypoint(m_prevWptIndex[2])) {
                  if (!(g_waypoint->GetPath(m_prevWptIndex[2])->flags &
                        WAYPOINT_LADDER)) {
                    foundGround = true;
                    previousNode = m_prevWptIndex[2];
                  } else if (IsValidWaypoint(m_prevWptIndex[3])) {
                    if (!(g_waypoint->GetPath(m_prevWptIndex[3])->flags &
                          WAYPOINT_LADDER)) {
                      foundGround = true;
                      previousNode = m_prevWptIndex[3];
                    }
                  }
                }
              }
            }

            if (foundGround) {
              FindPath(m_prevWptIndex[0], previousNode);
              break;
            }
          }
        }
      }
    }

    const Vector waypointOrigin = m_waypoint.origin;
    const float dist = (pev->origin - waypointOrigin).GetLengthSquared();
    if (m_waypointOrigin.z >= (pev->origin.z + 16.0f))
      m_waypointOrigin = waypointOrigin + Vector(0, 0, 16);
    else if (m_waypointOrigin.z < pev->origin.z + 16.0f && !IsOnLadder() &&
             IsOnFloor() && !(pev->flags & FL_DUCKING)) {
      m_moveSpeed = csqrtf(dist);
      if (m_moveSpeed < 150.0f)
        m_moveSpeed = 150.0f;
      else if (m_moveSpeed > pev->maxspeed)
        m_moveSpeed = pev->maxspeed;
    }

    if (dist < squaredf(24.0f))
      next = true;
  } else {
    if (m_waypoint.radius < 50 || (m_currentTravelFlags & PATHFLAG_JUMP)) {
      if (pev->flags & FL_DUCKING) {
        if ((pev->origin - m_destOrigin).GetLengthSquared2D() < squaredf(24.0f))
          next = true;
      } else {
        constexpr float MIN_RADIUS = 4.0f;
        const float checkRadiusSqr =
            squaredf(cmaxf(static_cast<float>(m_waypoint.radius), MIN_RADIUS));
        const Vector vel = pev->velocity.SkipZ();
        const float speed2 = vel.GetLengthSquared2D();
        if (speed2 < 1e-4f) {
          if ((pev->origin - m_destOrigin).GetLengthSquared2D() <
              checkRadiusSqr)
            next = true;
        } else {
          const Vector origin = pev->origin;
          const Vector toTarget = m_destOrigin - origin;
          float t = (toTarget.x * vel.x + toTarget.y * vel.y) / speed2;
          if (t < m_pathInterval)
            t = m_pathInterval;
          else {
            const float lookahead = cmaxf(m_pathInterval * 2.0f, 0.12f) *
                                    ((m_waypoint.radius < 5 ||
                                      (m_currentTravelFlags & PATHFLAG_JUMP))
                                         ? 1.5f
                                         : 1.0f);
            if (t > lookahead)
              t = lookahead;
          }

          const Vector closest = origin + vel * t;
          if ((closest - m_destOrigin).GetLengthSquared2D() < checkRadiusSqr)
            next = true;
          else if ((origin - m_destOrigin).GetLengthSquared2D() <
                   squaredf(cmaxf(static_cast<float>(m_waypoint.radius), 4.0f)))
            next = true;
        }
      }
    } else {
      if ((pev->origin - m_destOrigin).GetLengthSquared2D() <
          squaredf(static_cast<float>(m_waypoint.radius)))
        next = true;
    }
  }

  if (next) {
    // On ladder jump links, keep current node until we're actually near the
    // jump destination; otherwise current can switch too early.
    if (m_navNode.HasNext()) {
      const int16_t nextIndex = m_navNode.Next();
      for (int16_t i = 0; i < pMax; i++) {
        if (m_waypoint.index[i] != nextIndex)
          continue;

        const bool ladderJumpLink =
            (IsOnLadder() || (m_waypoint.flags & WAYPOINT_LADDER)) &&
            ((m_waypoint.connectionFlags[i] & PATHFLAG_JUMP) != 0);
        if (ladderJumpLink) {
          const Path *const nextPath = g_waypoint->GetPath(nextIndex);
          if ((pev->origin - nextPath->origin).GetLengthSquared() >
              squaredf(24.0f))
            next = false;
        }
        break;
      }
    }
  }

  if (next) {
    m_currentTravelFlags = 0;
    m_navNode.Shift();

    if (!m_navNode.IsEmpty()) {
      int16_t i;
      const int16_t destIndex = m_navNode.First();
      for (i = 0; i < pMax; i++) {
        if (m_waypoint.index[i] == destIndex) {
          m_prevTravelFlags = m_currentTravelFlags;
          m_currentTravelFlags = m_waypoint.connectionFlags[i];
          break;
        }
      }

      ChangeWptIndex(destIndex);
    }
  }
}

bool Bot::UpdateLiftHandling(void) {
  const float time = engine->GetTime();
  bool liftClosedDoorExists = false;

  TraceResult tr;

  // wait for something about for lift
  auto wait = [&]() {
    m_moveSpeed = 0.0f;
    m_strafeSpeed = 0.0f;
    m_buttons &= ~(IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT);
    ResetStuck();
  };

  // trace line to door
  TraceLine(pev->origin, m_waypoint.origin, TraceIgnore::Everything, m_myself,
            &tr);
  if (tr.flFraction < 1.0f &&
      (m_liftState == LiftState::None || m_liftState == LiftState::WaitingFor ||
       m_liftState == LiftState::LookingButtonOutside) &&
      !FNullEnt(tr.pHit) &&
      !cstrcmp(STRING(tr.pHit->v.classname), "func_door") &&
      pev->groundentity != tr.pHit) {
    if (m_liftState == LiftState::None) {
      m_liftState = LiftState::LookingButtonOutside;
      m_liftUsageTime = time + 7.0f;
    }

    liftClosedDoorExists = true;
  }

  if (!m_navNode.IsEmpty()) {
    // trace line down
    TraceLine(m_waypoint.origin, m_waypoint.origin + Vector(0.0f, 0.0f, -54.0f),
              TraceIgnore::Everything, m_myself, &tr);

    // if trace result shows us that it is a lift
    if (!liftClosedDoorExists && !FNullEnt(tr.pHit) &&
        (!cstrcmp(STRING(tr.pHit->v.classname), "func_door") ||
         !cstrcmp(STRING(tr.pHit->v.classname), "func_plat") ||
         !cstrcmp(STRING(tr.pHit->v.classname), "func_train"))) {
      if ((m_liftState == LiftState::None ||
           m_liftState == LiftState::WaitingFor ||
           m_liftState == LiftState::LookingButtonOutside) &&
          Math::FltZero(tr.pHit->v.velocity.z)) {
        if (cabsf(pev->origin.z - tr.vecEndPos.z) < 70.0f) {
          m_liftEntity = tr.pHit;
          m_liftState = LiftState::EnteringIn;
          m_liftTravelPos = m_waypoint.origin;
          m_liftUsageTime = time + 5.0f;
        }
      } else if (m_liftState == LiftState::TravelingBy) {
        m_liftState = LiftState::Leaving;
        m_liftUsageTime = time + 7.0f;
      }
    } else {
      if ((m_liftState == LiftState::None ||
           m_liftState == LiftState::WaitingFor) &&
          m_navNode.HasNext()) {
        const int16_t nextWaypoint = m_navNode.Next();
        if (IsValidWaypoint(nextWaypoint)) {
          const Path *pointer = g_waypoint->GetPath(nextWaypoint);
          if (pointer->flags & WAYPOINT_LIFT) {
            TraceLine(m_waypoint.origin, pointer->origin,
                      TraceIgnore::Everything, m_myself, &tr);
            if (!FNullEnt(tr.pHit) &&
                (!cstrcmp(STRING(tr.pHit->v.classname), "func_door") ||
                 !cstrcmp(STRING(tr.pHit->v.classname), "func_plat") ||
                 !cstrcmp(STRING(tr.pHit->v.classname), "func_train")))
              m_liftEntity = tr.pHit;
          }
        }

        m_liftState = LiftState::LookingButtonOutside;
        m_liftUsageTime = time + 15.0f;
      }
    }
  }

  // bot is going to enter the lift
  if (m_liftState == LiftState::EnteringIn) {
    m_destOrigin = m_liftTravelPos;

    // check if we enough to destination
    if ((pev->origin - m_destOrigin).GetLengthSquared() < squaredf(20.0f)) {
      wait();

      // need to wait our following teammate?
      bool needWaitForTeammate = false;

      // if some bot is following a bot going into lift - he should take the
      // same lift to go
      for (Bot *const &bot : g_botManager->m_bots) {
        if (!bot || !bot->m_isAlive || bot->m_team != m_team)
          continue;

        if (bot->pev->groundentity == m_liftEntity && bot->IsOnFloor())
          break;

        bot->m_liftEntity = m_liftEntity;
        bot->m_liftState = LiftState::EnteringIn;
        bot->m_liftTravelPos = m_liftTravelPos;

        needWaitForTeammate = true;
      }

      if (needWaitForTeammate) {
        m_liftState = LiftState::WaitingForTeammates;
        m_liftUsageTime = time + 8.0f;
      } else {
        m_liftState = LiftState::LookingButtonInside;
        m_liftUsageTime = time + 10.0f;
      }
    }
  }

  // bot is waiting for his teammates
  if (m_liftState == LiftState::WaitingForTeammates) {
    // need to wait our following teammate?
    bool needWaitForTeammate = false;

    for (Bot *const &bot : g_botManager->m_bots) {
      if (!bot || !bot->m_isAlive || bot->m_team != m_team ||
          bot->m_liftEntity != m_liftEntity)
        continue;

      if (bot->pev->groundentity == m_liftEntity || !bot->IsOnFloor()) {
        needWaitForTeammate = true;
        break;
      }
    }

    // need to wait for teammate
    if (needWaitForTeammate) {
      m_destOrigin = m_liftTravelPos;
      if ((pev->origin - m_destOrigin).GetLengthSquared() < squaredf(20.0f))
        wait();
    }

    // else we need to look for button
    if (!needWaitForTeammate || m_liftUsageTime < time) {
      m_liftState = LiftState::LookingButtonInside;
      m_liftUsageTime = time + 10.0f;
    }
  }

  // bot is trying to find button inside a lift
  if (m_liftState == LiftState::LookingButtonInside) {
    edict_t *button = FindNearestButton(STRING(m_liftEntity->v.targetname));

    // got a valid button entity?
    if (!FNullEnt(button) && pev->groundentity == m_liftEntity &&
        m_buttonPushTime + 1.0f < time &&
        Math::FltZero(m_liftEntity->v.velocity.z) && IsOnFloor()) {
      m_buttonEntity = button;
      SetProcess(Process::UseButton, "trying to use a button", false,
                 time + 10.0f);
    }
  }

  // is lift activated and bot is standing on it and lift is moving?
  if (m_liftState == LiftState::LookingButtonInside ||
      m_liftState == LiftState::EnteringIn ||
      m_liftState == LiftState::WaitingForTeammates ||
      m_liftState == LiftState::WaitingFor) {
    if (pev->groundentity == m_liftEntity &&
        !Math::FltZero(m_liftEntity->v.velocity.z) && IsOnFloor() &&
        (g_waypoint->GetPath(m_prevWptIndex[0])->flags & WAYPOINT_LIFT)) {
      m_liftState = LiftState::TravelingBy;
      m_liftUsageTime = time + 14.0f;

      if ((pev->origin - m_destOrigin).GetLengthSquared() < squaredf(20.0f))
        wait();
    }
  }

  // bots is currently moving on lift
  if (m_liftState == LiftState::TravelingBy) {
    m_destOrigin = Vector(m_liftTravelPos.x, m_liftTravelPos.y, pev->origin.z);
    if ((pev->origin - m_destOrigin).GetLengthSquared() < squaredf(20.0f))
      wait();
  }

  // need to find a button outside the lift
  if (m_liftState == LiftState::LookingButtonOutside) {
    // button has been pressed, lift should come
    if (m_buttonPushTime + 8.0f > time) {
      if (IsValidWaypoint(m_prevWptIndex[0]))
        m_destOrigin = g_waypoint->GetPath(m_prevWptIndex[0])->origin;
      else
        m_destOrigin = pev->origin;

      if ((pev->origin - m_destOrigin).GetLengthSquared() < squaredf(20.0f))
        wait();
    } else if (!FNullEnt(m_liftEntity)) {
      edict_t *button = FindNearestButton(STRING(m_liftEntity->v.targetname));

      // if we got a valid button entity
      if (!FNullEnt(button)) {
        // lift is already used?
        bool liftUsed = false;

        // iterate though clients, and find if lift already used
        for (const auto &client : g_clients) {
          if (FNullEnt(client.ent) || !(client.flags & CFLAG_USED) ||
              !(client.flags & CFLAG_ALIVE) || client.team != m_team ||
              client.ent == m_myself || FNullEnt(client.ent->v.groundentity))
            continue;

          if (client.ent->v.groundentity == m_liftEntity) {
            liftUsed = true;
            break;
          }
        }

        // lift is currently used
        if (liftUsed) {
          if (IsValidWaypoint(m_prevWptIndex[0]))
            m_destOrigin = g_waypoint->GetPath(m_prevWptIndex[0])->origin;
          else
            m_destOrigin = button->v.origin;

          if ((pev->origin - m_destOrigin).GetLengthSquared() < squaredf(20.0f))
            wait();
        } else {
          m_liftState = LiftState::WaitingFor;
          m_liftUsageTime = time + 20.0f;
          m_buttonEntity = button;
          SetProcess(Process::UseButton, "trying to use a button", false,
                     time + 10.0f);
        }
      } else {
        m_liftState = LiftState::WaitingFor;
        m_liftUsageTime = time + 15.0f;
      }
    }
  }

  // bot is waiting for lift
  if (m_liftState == LiftState::WaitingFor) {
    if (IsValidWaypoint(m_prevWptIndex[0])) {
      if (!(g_waypoint->GetPath(m_prevWptIndex[0])->flags & WAYPOINT_LIFT))
        m_destOrigin = g_waypoint->GetPath(m_prevWptIndex[0])->origin;
      else if (IsValidWaypoint(m_prevWptIndex[1]))
        m_destOrigin = g_waypoint->GetPath(m_prevWptIndex[1])->origin;
    }

    if ((pev->origin - m_destOrigin).GetLengthSquared() < squaredf(20.0f))
      wait();
  }

  // if bot is waiting for lift, or going to it
  if (m_liftState == LiftState::WaitingFor ||
      m_liftState == LiftState::EnteringIn) {
    // bot fall down somewhere inside the lift's groove :)
    if (pev->groundentity != m_liftEntity &&
        IsValidWaypoint(m_prevWptIndex[0])) {
      if (g_waypoint->GetPath(m_prevWptIndex[0])->flags & WAYPOINT_LIFT &&
          (m_waypoint.origin.z - pev->origin.z) > 50.0f &&
          (g_waypoint->GetPath(m_prevWptIndex[0])->origin.z - pev->origin.z) >
              50.0f) {
        m_liftState = LiftState::None;
        m_liftEntity = nullptr;
        m_liftUsageTime = 0.0f;

        if (IsValidWaypoint(m_prevWptIndex[2]))
          FindPath(m_currentWaypointIndex, m_prevWptIndex[2]);
        else {
          m_navNode.Clear();
          FindWaypoint();
        }

        return false;
      }
    }
  }

  return true;
}

bool Bot::UpdateLiftStates(void) {
  if (!FNullEnt(m_liftEntity) && !(m_waypoint.flags & WAYPOINT_LIFT)) {
    if (m_liftState == LiftState::TravelingBy) {
      m_liftState = LiftState::Leaving;
      m_liftUsageTime = engine->GetTime() + 10.0f;
    } else if (m_liftState == LiftState::Leaving &&
               m_liftUsageTime < engine->GetTime() &&
               pev->groundentity != m_liftEntity) {
      m_liftState = LiftState::None;
      m_liftUsageTime = 0.0f;
      m_liftEntity = nullptr;
    }
  }

  if (m_liftUsageTime < engine->GetTime() && !Math::FltZero(m_liftUsageTime)) {
    m_liftEntity = nullptr;
    m_liftState = LiftState::None;
    m_liftUsageTime = 0.0f;

    m_navNode.Clear();

    if (IsValidWaypoint(m_prevWptIndex[0])) {
      if (!(g_waypoint->GetPath(m_prevWptIndex[0])->flags & WAYPOINT_LIFT)) {
        ChangeWptIndex(m_prevWptIndex[0]);
        if (!m_navNode.IsEmpty())
          m_navNode.Set(0, m_prevWptIndex[0]);
      } else
        FindWaypoint();
    } else
      FindWaypoint();

    return false;
  }

  return true;
}

struct HeapNode {
  int16_t id{0};
  float priority{0.0f};
};
CArray<HeapNode> heapNode{2};

class PriorityQueue {
public:
  PriorityQueue(void);
  ~PriorityQueue(void);
  inline bool IsEmpty(void) const { return m_size < 1; }
  inline int16_t Size(void) const { return m_size; }
  inline void InsertLowest(const int16_t value, const float priority);
  inline int16_t RemoveLowest(void);
  inline bool Setup(void) {
    if (heapNode.NotAllocated()) {
      heapNode.Resize(g_numWaypoints, true);
      return false;
    }

    if (heapNode.Capacity() != g_numWaypoints) {
      heapNode.Resize(g_numWaypoints, true);
      return false;
    }

    m_size = 0;
    return true;
  }

private:
  int16_t m_size{0};
};

PriorityQueue::PriorityQueue(void) { m_size = 0; }

PriorityQueue::~PriorityQueue(void) { m_size = 0; }

// inserts a value into the priority queue
void PriorityQueue::InsertLowest(const int16_t value, const float priority) {
  heapNode[m_size].priority = priority;
  heapNode[m_size].id = value;

  int16_t child;
  int16_t parent;
  HeapNode temp;

  child = ++m_size - 1;
  while (child > 0) {
    parent = (child - 1) / 2;
    if (heapNode[parent].priority < heapNode[child].priority)
      break;

    temp = heapNode[child];
    heapNode[child] = heapNode[parent];
    heapNode[parent] = temp;
    child = parent;
  }
}

// removes the smallest item from the priority queue
int16_t PriorityQueue::RemoveLowest(void) {
  const int16_t retID = heapNode[0].id;

  m_size--;
  heapNode[0] = heapNode[m_size];

  int16_t rightChild;
  int16_t parent = 0;
  int16_t child = (2 * parent) + 1;
  HeapNode ref = heapNode[parent];
  while (child < m_size) {
    rightChild = (2 * parent) + 2;
    if (rightChild < m_size) {
      if (heapNode[rightChild].priority < heapNode[child].priority)
        child = rightChild;
    }

    if (ref.priority < heapNode[child].priority)
      break;

    heapNode[parent] = heapNode[child];
    parent = child;
    child = (2 * parent) + 1;
  }

  heapNode[parent] = ref;
  return retID;
}

inline const float HF_DistanceSquared(const int16_t &start,
                                      const int16_t &goal) {
  return (g_waypoint->m_paths[start].origin - g_waypoint->m_paths[goal].origin)
      .GetLengthSquared();
}

inline const float HF_Distance(const int16_t &start, const int16_t &goal) {
  return GetVectorDistanceSSE(g_waypoint->m_paths[start].origin,
                              g_waypoint->m_paths[goal].origin);
}

inline const float HF_Matrix(const int16_t &start, const int16_t &goal) {
  return static_cast<float>(
      *(g_waypoint->m_distMatrix.Get() + (start * g_numWaypoints) + goal));
}

inline const float HF_Auto(const int16_t &start, const int16_t &goal) {
  if (g_isMatrixReady)
    return HF_Matrix(start, goal);

  return HF_Distance(start, goal);
}

inline const float GF_CostHuman(const int16_t &index, const int16_t &parent,
                                const uint32_t &parentFlags, const int8_t &team,
                                const float &gravity, const bool &isZombie) {
  if (!parentFlags)
    return HF_Auto(index, parent);

  if (parentFlags & WAYPOINT_AVOID)
    return 65355.0f;

  if (isZombie) {
    if (parentFlags & WAYPOINT_HUMANONLY)
      return 65355.0f;
  } else {
    if (parentFlags & WAYPOINT_ZOMBIEONLY)
      return 65355.0f;

    if (parentFlags & WAYPOINT_DJUMP)
      return 65355.0f;
  }

  Path pathCache = g_waypoint->m_paths[parent];
  if (parentFlags & WAYPOINT_ONLYONE) {
    for (const auto &client : g_clients) {
      if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) ||
          team != client.team)
        continue;

      if ((client.origin - pathCache.origin).GetLengthSquared() <
          squaredi(static_cast<int>(pathCache.radius) + 64))
        return 65355.0f;
    }
  }

  float distance;
  float totalDistance = 0.0f;
  int8_t countCache = 0;
  for (const auto &client : g_clients) {
    if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) ||
        team == client.team || !IsZombieEntity(client.ent))
      continue;

    distance = ((client.ent->v.origin +
                 client.ent->v.velocity * g_pGlobals->frametime) -
                pathCache.origin)
                   .GetLengthSquared();
    if (distance < squaredi(static_cast<int>(pathCache.radius) + 128))
      countCache++;

    totalDistance += distance;
  }

  if (countCache && totalDistance > 0.0f)
    return (HF_Auto(index, parent) * static_cast<float>(countCache)) +
           totalDistance;

  return HF_Auto(index, parent);
}

inline const float GF_CostCareful(const int16_t &index, const int16_t &parent,
                                  const uint32_t &parentFlags,
                                  const int8_t &team, const float &gravity,
                                  const bool &isZombie) {
  if (!parentFlags)
    return HF_Auto(index, parent);

  if (parentFlags & WAYPOINT_AVOID)
    return 65355.0f;

  if (isZombie) {
    if (parentFlags & WAYPOINT_HUMANONLY)
      return 65355.0f;
  } else {
    if (parentFlags & WAYPOINT_ZOMBIEONLY)
      return 65355.0f;

    if (parentFlags & WAYPOINT_DJUMP)
      return 65355.0f;
  }

  if (parentFlags & WAYPOINT_ONLYONE) {
    Path pathCache = g_waypoint->m_paths[parent];
    for (const auto &client : g_clients) {
      if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) ||
          team != client.team)
        continue;

      if ((client.origin - pathCache.origin).GetLengthSquared() <
          squaredi(static_cast<int>(pathCache.radius) + 64))
        return 65355.0f;
    }
  }

  if (isZombie) {
    if (parentFlags & WAYPOINT_DJUMP) {
      Path pathCache = g_waypoint->m_paths[parent];
      int8_t countCache = 0;
      for (const auto &client : g_clients) {
        if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) ||
            client.team != team)
          continue;

        if ((client.origin - pathCache.origin).GetLengthSquared() <
            squaredi(static_cast<int>(pathCache.radius) + 512))
          countCache++;
        else if (IsVisible(pathCache.origin, client.ent))
          countCache++;
      }

      // don't count me
      if (countCache < static_cast<int8_t>(2))
        return 65355.0f;

      return HF_Auto(index, parent) / static_cast<float>(countCache);
    }
  }

  return HF_Auto(index, parent);
}

inline const float GF_CostNormal(const int16_t &index, const int16_t &parent,
                                 const uint32_t &parentFlags,
                                 const int8_t &team, const float &gravity,
                                 const bool &isZombie) {
  if (!parentFlags)
    return HF_Distance(index, parent);

  if (parentFlags & WAYPOINT_AVOID)
    return 65355.0f;

  if (isZombie) {
    if (parentFlags & WAYPOINT_HUMANONLY)
      return 65355.0f;
  } else {
    if (parentFlags & WAYPOINT_ZOMBIEONLY)
      return 65355.0f;

    if (parentFlags & WAYPOINT_DJUMP)
      return 65355.0f;
  }

  if (parentFlags & WAYPOINT_ONLYONE) {
    Path pathCache = g_waypoint->m_paths[parent];
    for (const auto &client : g_clients) {
      if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) ||
          team != client.team)
        continue;

      if ((client.origin - pathCache.origin).GetLengthSquared() <
          squaredi(static_cast<int>(pathCache.radius) + 64))
        return 65355.0f;
    }
  }

  if (isZombie) {
    if (parentFlags & WAYPOINT_DJUMP) {
      Path pathCache = g_waypoint->m_paths[parent];
      int8_t countCache = 0;
      for (const auto &client : g_clients) {
        if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) ||
            client.team != team)
          continue;

        if ((client.origin - pathCache.origin).GetLengthSquared() <
            squaredi(static_cast<int>(pathCache.radius) + 512))
          countCache++;
        else if (IsVisible(pathCache.origin, client.ent))
          countCache++;
      }

      // don't count me
      if (countCache < static_cast<int8_t>(2))
        return 65355.0f;

      return HF_Auto(index, parent) / static_cast<float>(countCache);
    }
  }

  if (parentFlags & WAYPOINT_LADDER)
    return HF_Auto(index, parent) * 2.0f;

  return HF_Auto(index, parent);
}

inline const float GF_CostRusher(const int16_t &index, const int16_t &parent,
                                 const uint32_t &parentFlags,
                                 const int8_t &team, const float &gravity,
                                 const bool &isZombie) {
  if (!parentFlags)
    return HF_Auto(index, parent);

  if (parentFlags & WAYPOINT_AVOID)
    return 65355.0f;

  if (isZombie) {
    if (parentFlags & WAYPOINT_HUMANONLY)
      return 65355.0f;
  } else {
    if (parentFlags & WAYPOINT_ZOMBIEONLY)
      return 65355.0f;

    if (parentFlags & WAYPOINT_DJUMP)
      return 65355.0f;
  }

  if (parentFlags & WAYPOINT_ONLYONE) {
    Path pathCache = g_waypoint->m_paths[parent];
    for (const auto &client : g_clients) {
      if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) ||
          team != client.team)
        continue;

      if ((client.origin - pathCache.origin).GetLengthSquared() <
          squaredi(static_cast<int>(pathCache.radius) + 64))
        return 65355.0f;
    }
  }

  // rusher bots never wait for boosting
  if (parentFlags & WAYPOINT_DJUMP)
    return 65355.0f;

  if (parentFlags & WAYPOINT_CROUCH)
    return HF_Auto(index, parent) * 2.0f;

  return HF_Auto(index, parent);
}

struct AStar {
  float g{0.0f};
  float f{0.0f};
  int16_t parent{-1};
  bool is_closed{false};
};
CArray<AStar> waypoints{2};

// this function finds a path from srcIndex to destIndex
void Bot::FindPath(int16_t &srcIndex, int16_t &destIndex) {
  if (!IsValidWaypoint(srcIndex))
    srcIndex = FindWaypoint();

  if (!IsValidWaypoint(srcIndex)) {
    m_navNode.Clear();
    return;
  }

  if (!IsValidWaypoint(destIndex)) {
    if (g_numWaypoints > 0)
      destIndex = static_cast<int16_t>(crandomint(0, g_numWaypoints - 1));
    else {
      m_navNode.Clear();
      return;
    }
  }

  if (ebot_force_shortest_path.GetBool() || g_numWaypoints > 2048) {
    FindShortestPath(srcIndex, destIndex);
    return;
  }

  if (srcIndex == destIndex)
    return;

  if (waypoints.NotAllocated()) {
    waypoints.Resize(g_numWaypoints, true);
    return;
  }

  if (waypoints.Capacity() != g_numWaypoints) {
    waypoints.Resize(g_numWaypoints, true);
    return;
  }

  PriorityQueue openList;
  if (!openList.Setup())
    return;

  const float (*hcalc)(const int16_t &, const int16_t &) = nullptr;
  const float (*gcalc)(const int16_t &, const int16_t &, const uint32_t &,
                       const int8_t &, const float &, const bool &) = nullptr;

  if (ebot_zombies_as_path_cost.GetBool() && !m_isZombieBot)
    gcalc = GF_CostHuman;
  else if (m_personality == Personality::Careful)
    gcalc = GF_CostCareful;
  else if (m_personality == Personality::Rusher)
    gcalc = GF_CostRusher;
  else
    gcalc = GF_CostNormal;

  if (!gcalc)
    return;

  if (g_isMatrixReady)
    hcalc = HF_Matrix;
  else
    hcalc = HF_Distance;

  if (!hcalc)
    return;

  Waypoint *gP = g_waypoint;
  if (!gP)
    return;

  int seed = m_index + m_numSpawns + m_currentWeapon;
  float min = ebot_pathfinder_seed_min.GetFloat();
  float max = ebot_pathfinder_seed_max.GetFloat();

  int16_t i;
  Path currPath;

  if (m_isStuck)
    seed += static_cast<int>(engine->GetTime() * 0.1f);

  for (i = 0; i < g_numWaypoints; i++) {
    waypoints[i].g = 0.0f;
    waypoints[i].f = 0.0f;
    waypoints[i].parent = -1;
    waypoints[i].is_closed = false;
  }

  // put start waypoint into open list
  AStar &srcWaypoint = waypoints[srcIndex];
  srcWaypoint.g = 0.0f;
  srcWaypoint.f = srcWaypoint.g + hcalc(srcIndex, destIndex);

  // loop cache
  AStar *currWaypoint;
  AStar *childWaypoint;
  int16_t currentIndex, self;
  uint32_t flags;
  float g, f;

  openList.InsertLowest(srcIndex, srcWaypoint.f);
  while (!openList.IsEmpty()) {
    currentIndex = openList.RemoveLowest();
    if (currentIndex == destIndex) {
      int16_t pathLength = 0;
      int16_t tempIndex = currentIndex;
      while (IsValidWaypoint(tempIndex)) {
        pathLength++;
        tempIndex = waypoints[tempIndex].parent;
      }

      if (!m_navNode.Init(pathLength))
        m_navNode.Clear();

      do {
        if (!m_navNode.Add(currentIndex))
          break;

        currentIndex = waypoints[currentIndex].parent;
      } while (IsValidWaypoint(currentIndex));

      m_navNode.Reverse();

      const int16_t firstIndex = m_navNode.First();
      bool adoptFirst = !IsValidWaypoint(m_currentWaypointIndex) ||
                        m_currentWaypointIndex == firstIndex;
      if (!adoptFirst && IsValidWaypoint(firstIndex)) {
        const Path *const firstPath = gP->GetPath(firstIndex);
        const bool ladderFirst = IsOnLadder() || (firstPath->flags & WAYPOINT_LADDER);
        const float threshold = ladderFirst ? 24.0f : 2048.0f;
        adoptFirst = (pev->origin - firstPath->origin).GetLengthSquared() <=
                     squaredf(threshold);
      }

      if (adoptFirst)
        ChangeWptIndex(firstIndex);
      return;
    }

    currWaypoint = &waypoints[currentIndex];
    if (currWaypoint->is_closed)
      continue;

    currWaypoint->is_closed = true;

    currPath = gP->m_paths[currentIndex];
    for (i = 0; i < pMax; i++) {
      self = currPath.index[i];
      if (!IsValidWaypoint(self))
        break;

      flags = gP->m_paths[self].flags;
      if (flags) {
        if (flags & WAYPOINT_FALLCHECK) {
          TraceResult tr;
          const Vector origin = gP->m_paths[self].origin;
          TraceLine(origin, origin - Vector(0.0f, 0.0f, 60.0f),
                    TraceIgnore::Nothing, m_myself, &tr);
          if (tr.flFraction >= 1.0f)
            continue;
        }

        if (flags & WAYPOINT_SPECIFICGRAVITY) {
          if ((pev->gravity * engine->GetGravity()) > gP->m_paths[self].gravity)
            continue;
        }

        if (flags & WAYPOINT_ONLYONE) {
          bool skip = false;
          for (Bot *const &bot : g_botManager->m_bots) {
            if (skip)
              break;

            if (bot && bot->m_isAlive && bot->m_myself != m_myself) {
              int16_t j;
              for (j = 0; j < bot->m_navNode.Length(); j++) {
                if (bot->m_navNode.Get(j) == self) {
                  skip = true;
                  break;
                }
              }
            }
          }

          if (skip)
            continue;
        }
      }

      g = currWaypoint->g + ((gcalc(currentIndex, self, flags, m_team,
                                    pev->gravity, m_isZombieBot) *
                              crandomfloatfast(seed, min, max)));
      f = g + hcalc(self, destIndex);

      childWaypoint = &waypoints[self];
      if (!childWaypoint->is_closed &&
          (childWaypoint->f == 0.0f || childWaypoint->f > f)) {
        childWaypoint->parent = currentIndex;
        childWaypoint->g = g;
        childWaypoint->f = f;
        openList.InsertLowest(self, f);
      }
    }
  }

  // roam around poorly :(
  if (m_navNode.IsEmpty()) {
    CArray<int16_t> PossiblePath;
    for (i = 0; i < g_numWaypoints; i++) {
      if (waypoints[i].is_closed)
        PossiblePath.Push(i);
    }

    if (!PossiblePath.IsEmpty()) {
      int16_t index = PossiblePath.Random();
      FindShortestPath(srcIndex, index);
      return;
    }

    FindShortestPath(srcIndex, destIndex);
  }
}

void Bot::FindShortestPath(int16_t &srcIndex, int16_t &destIndex) {
  if (!IsValidWaypoint(srcIndex))
    srcIndex = FindWaypoint();

  if (!IsValidWaypoint(srcIndex)) {
    m_navNode.Clear();
    return;
  }

  if (!IsValidWaypoint(destIndex)) {
    if (g_numWaypoints > 0)
      destIndex = static_cast<int16_t>(crandomint(0, g_numWaypoints - 1));
    else {
      m_navNode.Clear();
      return;
    }
  }

  if (srcIndex == destIndex)
    return;

  if (waypoints.NotAllocated()) {
    waypoints.Resize(g_numWaypoints, true);
    return;
  }

  if (waypoints.Capacity() != g_numWaypoints) {
    waypoints.Resize(g_numWaypoints, true);
    return;
  }

  PriorityQueue openList;
  if (!openList.Setup())
    return;

  const float (*hcalc)(const int16_t &, const int16_t &) = nullptr;

  if (g_isMatrixReady)
    hcalc = HF_Matrix;
  else
    hcalc = HF_DistanceSquared;

  if (!hcalc)
    return;

  Waypoint *gP = g_waypoint;
  if (!gP)
    return;

  int16_t i;
  for (i = 0; i < g_numWaypoints; i++) {
    waypoints[i].g = 0.0f;
    waypoints[i].f = 0.0f;
    waypoints[i].parent = -1;
    waypoints[i].is_closed = false;
  }

  AStar &srcWaypoint = waypoints[srcIndex];
  srcWaypoint.f = hcalc(srcIndex, destIndex);

  // loop cache
  AStar *currWaypoint;
  AStar *childWaypoint;
  int16_t currentIndex, self;
  uint32_t flags;
  Path currPath;
  float g, f;

  openList.InsertLowest(srcIndex, srcWaypoint.f);
  while (!openList.IsEmpty()) {
    currentIndex = openList.RemoveLowest();
    if (currentIndex == destIndex) {
      int16_t pathLength = 0;
      int16_t tempIndex = currentIndex;
      while (IsValidWaypoint(tempIndex)) {
        pathLength++;
        tempIndex = waypoints[tempIndex].parent;
      }

      if (!m_navNode.Init(pathLength))
        m_navNode.Clear();

      do {
        if (!m_navNode.Add(currentIndex))
          break;

        currentIndex = waypoints[currentIndex].parent;
      } while (IsValidWaypoint(currentIndex));

      m_navNode.Reverse();

      const int16_t firstIndex = m_navNode.First();
      bool adoptFirst = !IsValidWaypoint(m_currentWaypointIndex) ||
                        m_currentWaypointIndex == firstIndex;
      if (!adoptFirst && IsValidWaypoint(firstIndex)) {
        const Path *const firstPath = gP->GetPath(firstIndex);
        const bool ladderFirst = IsOnLadder() || (firstPath->flags & WAYPOINT_LADDER);
        const float threshold = ladderFirst ? 24.0f : 2048.0f;
        adoptFirst = (pev->origin - firstPath->origin).GetLengthSquared() <=
                     squaredf(threshold);
      }

      if (adoptFirst)
        ChangeWptIndex(firstIndex);
      return;
    }

    currWaypoint = &waypoints[currentIndex];
    if (currWaypoint->is_closed)
      continue;

    currWaypoint->is_closed = true;
    currPath = gP->m_paths[currentIndex];
    for (i = 0; i < pMax; i++) {
      self = currPath.index[i];
      if (!IsValidWaypoint(self))
        break;

      flags = gP->m_paths[self].flags;
      if (flags) {
        if (flags & WAYPOINT_FALLCHECK) {
          TraceResult tr;
          const Vector origin = gP->m_paths[self].origin;
          TraceLine(origin, origin - Vector(0.0f, 0.0f, 60.0f),
                    TraceIgnore::Nothing, m_myself, &tr);
          if (tr.flFraction >= 1.0f)
            continue;
        }

        if (flags & WAYPOINT_SPECIFICGRAVITY) {
          if ((pev->gravity * engine->GetGravity()) > gP->m_paths[self].gravity)
            continue;
        }

        if (flags & WAYPOINT_ONLYONE) {
          bool skip = false;
          for (Bot *const &bot : g_botManager->m_bots) {
            if (skip)
              break;

            if (bot && bot->m_isAlive && bot->m_myself != m_myself) {
              int16_t j;
              for (j = 0; j < bot->m_navNode.Length(); j++) {
                if (bot->m_navNode.Get(j) == self) {
                  skip = true;
                  break;
                }
              }
            }
          }

          if (skip)
            continue;
        }
      }

      g = currWaypoint->g + hcalc(self, currentIndex);
      f = g + hcalc(self, destIndex);
      childWaypoint = &waypoints[self];
      if (!childWaypoint->is_closed &&
          (childWaypoint->f == 0.0f || childWaypoint->f > f)) {
        childWaypoint->parent = currentIndex;
        childWaypoint->g = g;
        childWaypoint->f = f;
        openList.InsertLowest(self, f);
      }
    }
  }
}

void Bot::FindEscapePath(int16_t &srcIndex, const Vector &dangerOrigin) {
  if (!ebot_use_pathfinding_for_avoid.GetBool()) {
    m_navNode.Clear();
    return;
  }

  if (waypoints.NotAllocated()) {
    waypoints.Resize(g_numWaypoints, true);
    return;
  }

  if (waypoints.Capacity() != g_numWaypoints) {
    waypoints.Resize(g_numWaypoints, true);
    return;
  }

  PriorityQueue openList;
  if (!openList.Setup())
    return;

  Waypoint *gP = g_waypoint;
  if (!gP)
    return;

  if (!IsValidWaypoint(srcIndex)) {
    srcIndex = FindWaypoint();
    return;
  }

  int16_t i;
  for (i = 0; i < g_numWaypoints; i++) {
    waypoints[i].g = FLT_MAX;
    waypoints[i].f = FLT_MAX;
    waypoints[i].parent = -1;
    waypoints[i].is_closed = false;
  }

  int16_t currentIndex, neighborIndex;
  AStar *currWaypoint, *childWaypoint;
  Path currPath;
  uint32_t flags;

  AStar &srcWaypoint = waypoints[srcIndex];
  const Vector &srcPos = gP->m_paths[srcIndex].origin;
  srcWaypoint.g = (srcPos - pev->origin).GetLengthSquared();
  float dangerDistance = (srcPos - dangerOrigin).GetLengthSquared();
  srcWaypoint.f = srcWaypoint.g - dangerDistance;

  openList.InsertLowest(srcIndex, srcWaypoint.f);

  bool found = false;
  int16_t foundIndex = -1;
  int16_t iterations = 0;
  const int16_t maxIterations = 150;
  while (!openList.IsEmpty() && iterations < maxIterations) {
    iterations++;
    currentIndex = openList.RemoveLowest();
    currWaypoint = &waypoints[currentIndex];
    if (currWaypoint->is_closed)
      continue;

    currWaypoint->is_closed = true;
    currPath = gP->m_paths[currentIndex];
    if (!IsEnemyReachableToPosition(currPath.origin)) {
      found = true;
      foundIndex = currentIndex;
      break;
    }

    for (i = 0; i < pMax; i++) {
      neighborIndex = currPath.index[i];
      if (!IsValidWaypoint(neighborIndex))
        break;

      childWaypoint = &waypoints[neighborIndex];
      if (childWaypoint->is_closed)
        continue;

      flags = gP->m_paths[neighborIndex].flags;
      if (flags) {
        if (flags & WAYPOINT_FALLCHECK) {
          TraceResult tr;
          const Vector origin = gP->m_paths[neighborIndex].origin;
          TraceLine(origin, origin - Vector(0.0f, 0.0f, 60.0f),
                    TraceIgnore::Nothing, m_myself, &tr);
          if (tr.flFraction >= 1.0f)
            continue;
        }

        if (flags & WAYPOINT_SPECIFICGRAVITY) {
          if ((pev->gravity * engine->GetGravity()) >
              gP->m_paths[neighborIndex].gravity)
            continue;
        }

        if (flags & WAYPOINT_ONLYONE) {
          bool skip = false;
          for (Bot *const &bot : g_botManager->m_bots) {
            if (skip)
              break;
            if (bot && bot->m_isAlive && bot->m_myself != m_myself) {
              for (int16_t j = 0; j < bot->m_navNode.Length(); j++) {
                if (bot->m_navNode.Get(j) == neighborIndex) {
                  skip = true;
                  break;
                }
              }
            }
          }

          if (skip)
            continue;
        }
      }

      const Vector &neighborPos = gP->m_paths[neighborIndex].origin;
      float dangerDist = (neighborPos - dangerOrigin).GetLengthSquared();
      float dangerPenalty = 0.0f;
      constexpr float safeDist = 512.0f * 512.0f;
      if (dangerDist < safeDist)
        dangerPenalty = (safeDist - dangerDist) * 2.0f;

      float g = currWaypoint->g +
                (currPath.origin - neighborPos).GetLengthSquared() +
                dangerPenalty;
      float f = g + -dangerDist;
      if (!childWaypoint->is_closed &&
          (childWaypoint->f == 0.0f || childWaypoint->f > f)) {
        childWaypoint->parent = currentIndex;
        childWaypoint->g = g;
        childWaypoint->f = f;
        openList.InsertLowest(neighborIndex, f);
      }
    }
  }

  if (found && IsValidWaypoint(foundIndex)) {
    int16_t pathLength = 0;
    int16_t tempIndex = foundIndex;
    while (IsValidWaypoint(tempIndex) && pathLength < maxIterations) {
      pathLength++;
      tempIndex = waypoints[tempIndex].parent;
    }

    if (pathLength > m_navNode.GetCapacity()) {
      if (!m_navNode.Init(pathLength + 5)) {
        m_navNode.Clear();
        return;
      }
    } else
      m_navNode.Clear();

    int16_t currentIndex = foundIndex;
    int16_t safetyCounter = 0;

    do {
      safetyCounter++;
      if (safetyCounter > pathLength + 10) // Güvenlik
        break;

      if (!m_navNode.Add(currentIndex))
        break;

      currentIndex = waypoints[currentIndex].parent;
    } while (IsValidWaypoint(currentIndex));

    if (m_navNode.Length() > 1) {
      m_navNode.Reverse();
      const int16_t firstIndex = m_navNode.First();
      bool adoptFirst = !IsValidWaypoint(m_currentWaypointIndex) ||
                        m_currentWaypointIndex == firstIndex;
      if (!adoptFirst && IsValidWaypoint(firstIndex)) {
        const Path *const firstPath = gP->GetPath(firstIndex);
        const bool ladderFirst = IsOnLadder() || (firstPath->flags & WAYPOINT_LADDER);
        const float threshold = ladderFirst ? 24.0f : 2048.0f;
        adoptFirst = (pev->origin - firstPath->origin).GetLengthSquared() <=
                     squaredf(threshold);
      }

      if (adoptFirst)
        ChangeWptIndex(firstIndex);
    } else if (!m_navNode.IsEmpty()) {
      const int16_t firstIndex = m_navNode.First();
      bool adoptFirst = !IsValidWaypoint(m_currentWaypointIndex) ||
                        m_currentWaypointIndex == firstIndex;
      if (!adoptFirst && IsValidWaypoint(firstIndex)) {
        const Path *const firstPath = gP->GetPath(firstIndex);
        const bool ladderFirst = IsOnLadder() || (firstPath->flags & WAYPOINT_LADDER);
        const float threshold = ladderFirst ? 24.0f : 2048.0f;
        adoptFirst = (pev->origin - firstPath->origin).GetLengthSquared() <=
                     squaredf(threshold);
      }

      if (adoptFirst)
        ChangeWptIndex(firstIndex);
    }
  }
}

void Bot::CheckTouchEntity(edict_t *entity) {
  if (FNullEnt(entity) || !m_isAlive)
    return;

  if (entity == m_ignoreEntity || entity->v.takedamage == DAMAGE_NO)
    return;

  const float healthLimit = ebot_breakable_health_limit.GetFloat();
  const float health = entity->v.health;
  if (health <= 0.0f || health >= healthLimit)
    return;

  if (m_currentProcess == Process::DestroyBreakable && m_breakableEntity == entity)
    return;

  //player is ignored
  if (entity->v.flags & (FL_CLIENT | FL_FAKECLIENT))
      return;

  const bool builtInBreakable =
      FClassnameIs(entity, "func_breakable") ||
      (FClassnameIs(entity, "func_pushable") &&
       (entity->v.spawnflags & SF_PUSH_BREAKABLE)) ||
      FClassnameIs(entity, "func_wall");
  if (!builtInBreakable &&
      !IsExtraTouchBreakableClass(STRING(entity->v.classname)))
    return;

  TraceResult tr;
  TraceLine(EyePosition(), m_destOrigin, TraceIgnore::Nothing, m_myself, &tr);
  bool isBlocking = (!FNullEnt(tr.pHit) && tr.pHit == entity);

  if (!isBlocking) {
    TraceHull(EyePosition(), m_waypoint.origin, TraceIgnore::Nothing, head_hull,
              m_myself, &tr);
    isBlocking = (!FNullEnt(tr.pHit) && tr.pHit == entity);
  }

  if (!isBlocking)
    return;

  m_breakableEntity = entity;
  m_breakableOrigin = GetBoxOrigin(entity);

  const float time2 = engine->GetTime();
  if (!SetProcess(Process::DestroyBreakable, "trying to destroy a breakable",
                  false, time2 + 60.0f))
    return;


  //TODO; doesn't make sense here when this entity blocks the path
 /* if (pev->origin.z > m_breakableOrigin.z) // make bots smarter
  {
    // tell my enemies to destroy it, so i will fall
    edict_t *ent;
    TraceResult tr2;
    for (const auto &enemy : g_botManager->m_bots) {
      if (!enemy)
        continue;

      if (m_team == enemy->m_team)
        continue;

      if (!enemy->m_isAlive)
        continue;

      if (enemy->m_isZombieBot)
        continue;

      if (enemy->m_currentWeapon == Weapon::Knife)
        continue;

      ent = enemy->m_myself;
      if (FNullEnt(ent))
        continue;

      TraceHull(enemy->EyePosition(), m_breakableOrigin, TraceIgnore::Nothing,
                point_hull, ent, &tr);
      TraceHull(ent->v.origin, m_breakableOrigin, TraceIgnore::Nothing,
                head_hull, ent, &tr2);
      if ((!FNullEnt(tr.pHit) && tr.pHit == entity) ||
          (!FNullEnt(tr2.pHit) && tr2.pHit == entity)) {
        enemy->m_breakableEntity = entity;
        enemy->m_breakableOrigin = m_breakableOrigin;
        enemy->SetProcess(Process::DestroyBreakable,
                          "trying to destroy a breakable for my enemy fall",
                          false, time2 + 60.0f);
      }
    }
  } else if (!m_isZombieBot) // tell my friends to destroy it
  {
    edict_t *ent;
    TraceResult tr2;
    for (Bot *const &bot : g_botManager->m_bots) {
      if (!bot)
        continue;

      if (m_team != bot->m_team)
        continue;

      if (!bot->m_isAlive)
        continue;

      if (bot->m_isZombieBot)
        continue;

      ent = bot->m_myself;
      if (FNullEnt(ent))
        continue;

      if (m_myself == ent)
        continue;

      TraceHull(bot->EyePosition(), m_breakableOrigin, TraceIgnore::Nothing,
                point_hull, ent, &tr);
      TraceHull(ent->v.origin, m_breakableOrigin, TraceIgnore::Nothing,
                head_hull, ent, &tr2);

      if ((!FNullEnt(tr.pHit) && tr.pHit == entity) ||
          (!FNullEnt(tr2.pHit) && tr2.pHit == entity)) {
        bot->m_breakableEntity = entity;
        bot->m_breakableOrigin = m_breakableOrigin;
        bot->SetProcess(Process::DestroyBreakable,
                        "trying to help my friend for destroy a breakable",
                        false, time2 + 60.0f);
      }
    }
  }
  */
}

int16_t Bot::FindWaypoint(void) {
  if (m_isAlive) {
    auto isLadderDistanceValid = [&](const int16_t candidate) {
      if (!IsValidWaypoint(candidate))
        return false;

      const Path *const path = g_waypoint->GetPath(candidate);
      const bool hasCurrentWaypoint = IsValidWaypoint(m_currentWaypointIndex);

      // Recovery mode: when we are on a ladder but current waypoint is invalid,
      // do not constrain acquisition to the tight ladder radius.
      if (IsOnLadder() && !hasCurrentWaypoint)
        return true;

      const bool ladderContext =
          IsOnLadder() || (hasCurrentWaypoint && (m_waypoint.flags & WAYPOINT_LADDER));
      if (!(path->flags & WAYPOINT_LADDER) && !ladderContext)
        return true;

      constexpr float LADDER_FIND_THRESHOLD = 24.0f;
      return (pev->origin - path->origin).GetLengthSquared() <=
             squaredf(LADDER_FIND_THRESHOLD);
    };

    auto isReachableCandidate = [&](const int16_t candidate) {
      if (!IsValidWaypoint(candidate) ||
          !g_waypoint->Reachable(m_myself, candidate))
        return false;

      // Avoid snapping to ladder waypoints that are still too far.
      return isLadderDistanceValid(candidate);
    };

    int16_t index;
    if (!m_isStuck && m_navNode.HasNext() &&
        isReachableCandidate(m_navNode.First()))
      index = m_navNode.First();
    else if (!m_isStuck && isReachableCandidate(g_clients[m_index].wp))
      index = g_clients[m_index].wp;
    else {
      index = g_waypoint->FindNearestToEnt(pev->origin, 2048.0f, m_myself);
      if (!isLadderDistanceValid(index))
        index = -1;

      if (!IsValidWaypoint(index)) {
        index = g_waypoint->FindNearest(pev->origin);
        if (!isLadderDistanceValid(index))
          index = -1;
      }

      if (!IsValidWaypoint(index) && IsValidWaypoint(m_currentWaypointIndex))
        index = m_currentWaypointIndex;
    }

    ChangeWptIndex(index);
    return index;
  }

  return m_currentWaypointIndex;
}

// return priority of player (0 = max pri)
inline int GetPlayerPriority(edict_t *player) {
  // human players have highest priority
  const Bot *bot = g_botManager->GetBot(player);
  if (bot)
    return bot->m_index + 3; // everyone else is ranked by their unique ID
                             // (which cannot be zero)

  return 1;
}

void Bot::ResetStuck(void) {
  m_stuckTime = 0.0f;
  m_isStuck = false;
  m_checkFall = false;
  for (Vector &fall : m_checkFallPoint)
    fall = nullvec;

  ResetCollideState();
  IgnoreCollisionShortly();
}

void Bot::IgnoreCollisionShortly(void) {
  const float time2 = engine->GetTime();
  m_prevTime = time2 + 0.25f;
  m_lastCollTime = time2 + 2.0f;
  if (m_waypointTime < time2 + 7.0f)
    m_waypointTime = time2 + 7.0f;

  m_isStuck = false;
  m_stuckTime = 0.0f;
}

bool Bot::CheckWaypoint(void) {
  if (m_isSlowThink) {
    const bool ladderTraverse = IsOnLadder() || (m_waypoint.flags & WAYPOINT_LADDER);
    if ((m_isStuck && m_stuckTime > 7.0f) ||
        (!ladderTraverse &&
         ((pev->origin - m_waypoint.origin).GetLengthSquared() >
              squaredf(512.0f) ||
          !g_waypoint->Reachable(m_myself, m_currentWaypointIndex)))) {
      FindWaypoint();
      FindPath(m_currentWaypointIndex, m_currentGoalIndex);
      return false;
    }

    if (m_checkFall) {
      if (IsOnFloor()) {
        m_checkFall = false;
        bool fixFall = false;

        const float baseDistance =
            (m_checkFallPoint[0] - m_checkFallPoint[1]).GetLengthSquared();
        const float nowDistance =
            (pev->origin - m_checkFallPoint[1]).GetLengthSquared();

        if (nowDistance > baseDistance &&
            (nowDistance > (baseDistance * 1.25f) ||
             nowDistance > (baseDistance + 200.0f)) &&
            baseDistance > squaredf(80.0f) && nowDistance > squaredf(100.0f))
          fixFall = true;
        else if (pev->origin.z + 128.0f < m_checkFallPoint[1].z &&
                 pev->origin.z + 128.0f < m_checkFallPoint[0].z)
          fixFall = true;
        else if (IsValidWaypoint(m_currentWaypointIndex) &&
                 nowDistance < squaredf(32.0f) &&
                 pev->origin.z + 64.0f < m_checkFallPoint[1].z)
          fixFall = true;

        if (fixFall) {
          FindWaypoint();
          FindPath(m_currentWaypointIndex, m_currentGoalIndex);
        }
      }

    } else if (IsOnFloor()) {
      m_checkFallPoint[0] = pev->origin;
      if (m_hasEnemiesNear && !FNullEnt(m_nearestEnemy))
        m_checkFallPoint[1] = m_nearestEnemy->v.origin;
      else if (IsValidWaypoint(m_currentWaypointIndex))
        m_checkFallPoint[1] = m_destOrigin;
      else
        m_checkFallPoint[1] = nullvec;
    } else if (!IsOnLadder() && !IsInWater()) {
      if (!m_checkFallPoint[0].IsNull() && !m_checkFallPoint[1].IsNull())
        m_checkFall = true;
    }
  }

  return true;
}

void Bot::CheckStuck(const Vector &directionNormal, const float finterval) {
  if (m_waypoint.flags & WAYPOINT_FALLCHECK) {
    ResetStuck();
    return;
  }

  if (m_prevTime < engine->GetTime()) {
    m_isStuck = false;

    // see how far bot has moved since the previous position...
    m_movedDistance = (m_prevOrigin - pev->origin).GetLengthSquared();

    // save current position as previous
    m_prevOrigin = pev->origin;

    if (pev->flags & FL_DUCKING)
      m_prevTime = engine->GetTime() + 0.55f;
    else
      m_prevTime = engine->GetTime() + 0.25f;
  } else {
    // reset path if current waypoint is too far
    if (m_hasFriendsNear && !ebot_has_semiclip.GetBool() &&
        !FNullEnt(m_nearestFriend)) {
      m_avoid = nullptr;
      const int prio = GetPlayerPriority(m_myself);
      int friendPrio = GetPlayerPriority(m_nearestFriend);
      if (prio > friendPrio)
        m_avoid = m_nearestFriend;
      else {
        const Bot *bot = g_botManager->GetBot(m_nearestFriend);
        if (bot && !FNullEnt(bot->m_avoid) && m_myself != bot->m_avoid &&
            friendPrio > GetPlayerPriority(bot->m_avoid))
          m_avoid = bot->m_avoid;
      }

      if (!FNullEnt(m_avoid)) {
        const float interval =
            finterval * (!(pev->flags & FL_DUCKING) &&
                                 pev->velocity.GetLengthSquared2D() > 200.0f
                             ? 10.0f
                             : 5.0f);

        // use our movement angles, try to predict where we should be next frame
        Vector right, forward;
        m_moveAngles.BuildVectors(&forward, &right, nullptr);

        Vector predict = pev->origin + forward * m_moveSpeed * interval;

        predict += right * m_strafeSpeed * interval;
        predict += pev->velocity * interval;

        const float distance =
            (pev->origin - m_avoid->v.origin).GetLengthSquared();
        const float movedDistance =
            (predict - m_avoid->v.origin).GetLengthSquared();
        const float nextFrameDistance =
            (pev->origin - (m_avoid->v.origin + m_avoid->v.velocity * interval))
                .GetLengthSquared();

        // is player that near now or in future that we need to steer away?
        if (movedDistance < squaredf(64.0f) ||
            (distance < squaredf(72.0f) && nextFrameDistance < distance)) {
          const Vector dir = (pev->origin - m_avoid->v.origin).Normalize2D();

          // to start strafing, we have to first figure out if the target is on
          // the left side or right side
          if ((dir | right.Normalize2D()) > 0.0f)
            SetStrafeSpeed(directionNormal, pev->maxspeed);
          else
            SetStrafeSpeed(directionNormal, -pev->maxspeed);

          if (distance < squaredf(80.0f)) {
            if ((dir | forward.Normalize2D()) < 0.0f)
              m_moveSpeed = -pev->maxspeed;
            else
              m_moveSpeed = pev->maxspeed;
          }

          if (m_isStuck && m_navNode.HasNext()) {
            const Bot *bot = g_botManager->GetBot(m_avoid);
            if (bot && bot != this && bot->m_navNode.HasNext()) {
              if (m_currentWaypointIndex == bot->m_currentWaypointIndex) {
              }
              /*else // TODO: find better solution
              {
                      m_currentGoalIndex = bot->m_currentGoalIndex;
                      m_currentWaypointIndex = bot->m_currentWaypointIndex;
                      FindShortestPath(m_currentWaypointIndex,
              m_currentGoalIndex);
              }*/
            }
          }
        }
      }
    }
  }

  if (m_lastCollTime < engine->GetTime()) {
    if (m_movedDistance < 2.0f &&
        (m_prevSpeed > 20.0f ||
         m_prevVelocity < squaredf(m_moveSpeed * 0.5f))) {
      m_prevTime = engine->GetTime();
      m_isStuck = true;

      if (Math::FltZero(m_firstCollideTime))
        m_firstCollideTime = engine->GetTime() + 0.2f;
    } else {
      // test if there's something ahead blocking the way
      if (!(m_waypoint.flags & WAYPOINT_LADDER) && !IsOnLadder() &&
          CantMoveForward(directionNormal)) {
        if (Math::FltZero(m_firstCollideTime))
          m_firstCollideTime = engine->GetTime() + 0.2f;
        else if (m_firstCollideTime < engine->GetTime())
          m_isStuck = true;
      } else
        m_firstCollideTime = 0.0f;
    }
  }

  if (m_isStuck) {
    if (m_isEnemyReachable)
      CheckReachable();
    else {
      m_stuckTime += finterval;
      if (m_stuckTime > 60.0f)
        Kill();
    }

    // not yet decided what to do?
    if (m_collisionState == COSTATE_UNDECIDED) {
      TraceResult tr;
      uint32_t bits = 0;

      if (IsOnLadder())
        bits |= COPROBE_STRAFE;
      else if (IsInWater())
        bits |= (COPROBE_JUMP | COPROBE_STRAFE);
      else {
        bits |= COPROBE_STRAFE;
        if (pev->maxspeed > 20.0f) {
          if (g_numWaypoints > 1024) {
            if (chanceof(70))
              bits |= COPROBE_DUCK;
          } else if (chanceof(10))
            bits |= COPROBE_DUCK;

          if (chanceof(35))
            bits |= COPROBE_JUMP;
        }
      }

      // collision check allowed if not flying through the air
      if (IsOnFloor() || IsOnLadder() || IsInWater()) {
        Vector src, dest;

        int state[8];
        int i = 0;

        // first 4 entries hold the possible collision states
        state[i++] = COSTATE_STRAFELEFT;
        state[i++] = COSTATE_STRAFERIGHT;
        state[i++] = COSTATE_JUMP;
        state[i++] = COSTATE_DUCK;

        if (bits & COPROBE_STRAFE) {
          state[i] = 0;
          state[i + 1] = 0;

          // to start strafing, we have to first figure out if the target is on
          // the left side or right side
          MakeVectors(m_moveAngles);

          bool dirRight = false;
          bool dirLeft = false;
          bool blockedLeft = false;
          bool blockedRight = false;

          if (((pev->origin - m_destOrigin).Normalize2D() |
               g_pGlobals->v_right.Normalize2D()) > 0.0f)
            dirRight = true;
          else
            dirLeft = true;

          const Vector testDir = m_moveSpeed > 0.0f ? g_pGlobals->v_forward
                                                    : -g_pGlobals->v_forward;

          // now check which side is blocked
          src = pev->origin + g_pGlobals->v_right * 52.0f;
          dest = src + testDir * 52.0f;

          TraceHull(src, dest, TraceIgnore::Nothing, head_hull, m_myself, &tr);
          if (tr.flFraction < 1.0f)
            blockedRight = true;

          src = pev->origin - g_pGlobals->v_right * 52.0f;
          dest = src + testDir * 52.0f;

          TraceHull(src, dest, TraceIgnore::Nothing, head_hull, m_myself, &tr);
          if (tr.flFraction < 1.0f)
            blockedLeft = true;

          if (dirLeft)
            state[i] += 5;
          else
            state[i] -= 5;

          if (blockedLeft)
            state[i] -= 5;

          i++;

          if (dirRight)
            state[i] += 5;
          else
            state[i] -= 5;

          if (blockedRight)
            state[i] -= 5;
        }

        // now weight all possible states
        if (bits & COPROBE_JUMP) {
          state[i] = 0;
          if (CanJumpUp(directionNormal))
            state[i] += 10;

          if (m_destOrigin.z >= pev->origin.z + 18.0f)
            state[i] += 5;

          if (IsVisible(m_destOrigin, m_myself)) {
            MakeVectors(m_moveAngles);

            src = EyePosition();
            src = src + g_pGlobals->v_right * 15.0f;

            TraceHull(src, m_destOrigin, TraceIgnore::Nothing, point_hull,
                      m_myself, &tr);
            if (tr.flFraction >= 1.0f) {
              src = EyePosition();
              src = src - g_pGlobals->v_right * 15.0f;

              TraceHull(src, m_destOrigin, TraceIgnore::Nothing, point_hull,
                        m_myself, &tr);
              if (tr.flFraction >= 1.0f)
                state[i] += 5;
            }
          }

          if (pev->flags & FL_DUCKING)
            src = pev->origin;
          else
            src = pev->origin + Vector(0.0f, 0.0f, -17.0f);

          dest = src + directionNormal * 30.0f;
          TraceHull(src, dest, TraceIgnore::Nothing, point_hull, m_myself, &tr);

          if (tr.flFraction < 1.0f)
            state[i] += 10;
        } else
          state[i] = 0;

        i++;

        if (bits & COPROBE_DUCK) {
          state[i] = 0;
          if (CanDuckUnder(directionNormal))
            state[i] += 10;

          if ((m_destOrigin.z + 36.0f <= pev->origin.z) &&
              IsVisible(m_destOrigin, m_myself))
            state[i] += 5;
        } else
          state[i] = 0;

        i++;

        int temp;
        bool isSorting = false;

        // weighted all possible moves, now sort them to start with most
        // probable
        do {
          isSorting = false;
          for (i = 0; i < 3; i++) {
            if (state[i + 3] < state[i + 3 + 1]) {
              temp = state[i];
              state[i] = state[i + 1];
              state[i + 1] = temp;
              temp = state[i + 3];
              state[i + 3] = state[i + 4];
              state[i + 4] = temp;
              isSorting = true;
            }
          }
        } while (isSorting);

        for (i = 0; i < 3; i++)
          m_collideMoves[i] = state[i];

        m_collideTime = engine->GetTime();
        m_probeTime = engine->GetTime() + crandomfloat(0.5f, 1.5f);
        m_collisionProbeBits = bits;
        m_collisionState = COSTATE_PROBING;
        m_collStateIndex = 0;
      }
    } else if (m_collisionState == COSTATE_PROBING) {
      if (m_probeTime < engine->GetTime()) {
        m_collStateIndex++;
        m_probeTime = engine->GetTime() + crandomfloat(0.5f, 1.5f);
        if (m_collStateIndex > 3)
          ResetCollideState();
      } else if (m_collStateIndex < 3) {
        switch (m_collideMoves[m_collStateIndex]) {
        case COSTATE_JUMP: {
          if (pev->maxspeed < 20.0f)
            break;

          if (IsOnLadder())
            break;

          if (m_isZombieBot && pev->flags & FL_DUCKING) {
            m_moveSpeed = pev->maxspeed;
            m_buttons = IN_FORWARD;
          } else {
            m_buttons |= (IN_DUCK | IN_JUMP);
            TryStartDoubleJump(this, m_waypoint.origin);
          }

          break;
        }
        case COSTATE_DUCK: {
          if (pev->maxspeed < 20.0f)
            break;

          m_buttons |= IN_DUCK;
          break;
        }
        case COSTATE_STRAFELEFT: {
          m_buttons |= IN_MOVELEFT;
          SetStrafeSpeedNoCost(directionNormal, -pev->maxspeed);
          break;
        }
        case COSTATE_STRAFERIGHT: {
          m_buttons |= IN_MOVERIGHT;
          SetStrafeSpeedNoCost(directionNormal, pev->maxspeed);
          break;
        }
        }
      }
    }
  } else {
    m_stuckTime -= finterval;
    if (m_stuckTime < 0.0f)
      m_stuckTime = 0.0f;

    // boosting improve
    if (m_isZombieBot && m_waypoint.flags & WAYPOINT_DJUMP && IsOnFloor() &&
        ((pev->origin - m_waypoint.origin).GetLengthSquared() <
         squaredf(54.0f)))
      m_buttons |= IN_DUCK;
    else {
      if (m_probeTime + 1.0f < engine->GetTime())
        ResetCollideState(); // reset collision memory if not being stuck for 1
                             // sec
      else {
        // remember to keep pressing duck if it was necessary ago
        if (m_collideMoves[m_collStateIndex] == COSTATE_DUCK && IsOnFloor() ||
            IsInWater())
          m_buttons |= IN_DUCK;
        else if (m_collideMoves[m_collStateIndex] == COSTATE_STRAFELEFT)
          SetStrafeSpeedNoCost(directionNormal, -pev->maxspeed);
        else if (m_collideMoves[m_collStateIndex] == COSTATE_STRAFERIGHT)
          SetStrafeSpeedNoCost(directionNormal, pev->maxspeed);
      }
    }
  }

  m_prevSpeed = cabsf(m_moveSpeed);
  m_prevVelocity = pev->velocity.GetLengthSquared2D();
}

void Bot::ResetCollideState(void) {
  m_probeTime = 0.0f;
  m_collideTime = 0.0f;

  m_collisionState = COSTATE_UNDECIDED;
  m_collStateIndex = 0;

  for (int &collide : m_collideMoves)
    collide = 0;
}

void Bot::SetWaypointOrigin(void) {
  if (m_currentTravelFlags & PATHFLAG_JUMP ||
      m_waypoint.flags & WAYPOINT_FALLRISK || m_isStuck) {
    m_waypointOrigin = m_waypoint.origin;
    if (!IsOnLadder() && !IsInWater()) {
      // hack, let bot turn instantly to next waypoint
      const float speed = pev->velocity.GetLength2D();
      if (speed > 10.0f) {
        const Vector vel = (m_waypoint.origin - pev->origin).Normalize2D();
        if (!vel.IsNull()) {
          pev->velocity.x = vel.x * speed;
          pev->velocity.y = vel.y * speed;
        }
      }
    }

    if (m_currentTravelFlags & PATHFLAG_JUMP)
      m_jumpReady = true;
  } else if (m_waypoint.radius) {
    const float radius = static_cast<float>(m_waypoint.radius);
    if (m_navNode.HasNext()) {
      // until we find better alternative just use this m_avoid
      if (m_hasFriendsNear) {
        if (!ebot_has_semiclip.GetBool() && !FNullEnt(m_avoid)) {
          if (m_isStuck)
            MakeVectors((m_waypoint.origin - pev->origin).ToAngles());
          else
            MakeVectors((g_waypoint->m_paths[m_navNode.Next()].origin -
                         m_waypoint.origin)
                            .ToAngles());

          const Vector forward = g_pGlobals->v_forward;
          const Vector right = Vector(-forward.y, forward.x, 0.0f);

          // if we need to avoid someone, move to the side instead of blocking
          if (!FNullEnt(m_avoid)) {
            const Bot *avoidBot = g_botManager->GetBot(m_avoid);
            Vector toAvoid;

            if (avoidBot) // we know exactly where they're going, use their
                          // target position to predict their path
              toAvoid = (avoidBot->m_waypointOrigin - m_waypoint.origin)
                            .Normalize2D();
            else // guess based on current position
              toAvoid = (m_avoid->v.origin - m_waypoint.origin).Normalize2D();

            // check which side is better to avoid (hitbox + margin)
            const float sideOffset =
                ((toAvoid | right) > 0.0f) ? -48.0f : 48.0f;
            m_waypointOrigin.x =
                m_waypoint.origin.x + forward.x * radius + right.x * sideOffset;
            m_waypointOrigin.y =
                m_waypoint.origin.y + forward.y * radius + right.y * sideOffset;
            m_waypointOrigin.z = m_waypoint.origin.z;
          } else {
            const Vector direction =
                (forward * (1.0f - cabsf(m_sidePreference) * 0.5f) +
                 Vector(-forward.y, forward.x, 0.0f) * m_sidePreference)
                    .Normalize2D();
            m_waypointOrigin.x = m_waypoint.origin.x + direction.x * radius;
            m_waypointOrigin.y = m_waypoint.origin.y + direction.y * radius;
            m_waypointOrigin.z = m_waypoint.origin.z;
          }
        } else {
          if (m_isStuck)
            MakeVectors((m_waypoint.origin - pev->origin).ToAngles());
          else
            MakeVectors((g_waypoint->m_paths[m_navNode.Next()].origin -
                         m_waypoint.origin)
                            .ToAngles());

          const Vector forward = g_pGlobals->v_forward;
          const Vector direction =
              (forward * (1.0f - cabsf(m_sidePreference) * 0.5f) +
               Vector(-forward.y, forward.x, 0.0f) * m_sidePreference)
                  .Normalize2D();
          m_waypointOrigin.x = m_waypoint.origin.x + direction.x * radius;
          m_waypointOrigin.y = m_waypoint.origin.y + direction.y * radius;
          m_waypointOrigin.z = m_waypoint.origin.z;
        }
      } else {
        if (m_isStuck)
          MakeVectors((m_waypoint.origin - pev->origin).ToAngles());
        else
          MakeVectors(
              (g_waypoint->m_paths[m_navNode.Next()].origin - m_waypoint.origin)
                  .ToAngles());

        const Vector forward = g_pGlobals->v_forward;
        m_waypointOrigin.x = m_waypoint.origin.x + forward.x * radius;
        m_waypointOrigin.y = m_waypoint.origin.y + forward.y * radius;
        m_waypointOrigin.z = m_waypoint.origin.z;
      }
    } else {
      m_waypointOrigin.x = m_waypoint.origin.x + crandomfloat(-radius, radius);
      m_waypointOrigin.y = m_waypoint.origin.y + crandomfloat(-radius, radius);
      m_waypointOrigin.z = m_waypoint.origin.z;

      if (m_hasFriendsNear) {
        const float range =
            static_cast<float>(m_numFriendsNear * m_numFriendsNear);
        m_waypointOrigin.x += crandomfloat(-range, range);
        m_waypointOrigin.y += crandomfloat(-range, range);
      }
    }
  } else
    m_waypointOrigin = m_waypoint.origin;
}

void Bot::ChangeWptIndex(const int16_t waypointIndex) {
  const int16_t oldIndex = m_currentWaypointIndex;
  m_currentWaypointIndex = waypointIndex;
  if (!IsValidWaypoint(waypointIndex))
    return;

  if (m_prevWptIndex[0] != m_currentWaypointIndex) {
    m_prevWptIndex[3] = m_prevWptIndex[2];
    m_prevWptIndex[2] = m_prevWptIndex[1];
    m_prevWptIndex[1] = m_prevWptIndex[0];
    m_prevWptIndex[0] = m_currentWaypointIndex;
  }

  m_waypoint = g_waypoint->m_paths[waypointIndex];
  if (ebot_debug.GetBool() && oldIndex != waypointIndex &&
      (m_waypoint.flags & WAYPOINT_LADDER)) {
    const float distToCurrentWp =
        (pev->origin - m_waypoint.origin).GetLength();
    ServerPrint("%s ladder current wp changed: %d -> %d (dist %.1f)",
                GetEntityName(m_myself), oldIndex, waypointIndex,
                distToCurrentWp);
  }

  SetWaypointOrigin();
  const float speed = pev->velocity.GetLength();
  if (speed > 10.0f)
    m_waypointTime =
        engine->GetTime() +
        cclampf(GetVectorDistanceSSE(pev->origin, m_destOrigin) / speed, 3.0f,
                12.0f);
  else
    m_waypointTime = engine->GetTime() + 12.0f;
}

// checks if bot is blocked in his movement direction (excluding doors)
bool Bot::CantMoveForward(const Vector &normal) {
  // first do a trace from the bot's eyes forward...
  const Vector eyePosition = EyePosition();
  Vector src = eyePosition;
  Vector forward = src + normal * 24.0f;

  MakeVectors(Vector(0.0f, pev->angles.y, 0.0f));
  TraceResult tr;

  // trace from the bot's eyes straight forward...
  TraceLine(src, forward, TraceIgnore::Nothing, m_myself, &tr);

  // check if the trace hit something...
  if (tr.flFraction < 1.0f) {
    if (!cstrncmp("func_door", STRING(tr.pHit->v.classname), 9))
      return false;

    return true; // bot's head will hit something
  }

  // bot's head is clear, check at shoulder level...
  // trace from the bot's shoulder left diagonal forward to the right
  // shoulder...
  src = eyePosition + Vector(0.0f, 0.0f, -16.0f) - g_pGlobals->v_right * 16.0f;
  forward = eyePosition + Vector(0.0f, 0.0f, -16.0f) +
            g_pGlobals->v_right * 16.0f + normal * 24.0f;

  TraceLine(src, forward, TraceIgnore::Nothing, m_myself, &tr);

  // check if the trace hit something...
  if (tr.flFraction < 1.0f &&
      cstrncmp("func_door", STRING(tr.pHit->v.classname), 9))
    return true; // bot's body will hit something

  // bot's head is clear, check at shoulder level...
  // trace from the bot's shoulder right diagonal forward to the left
  // shoulder...
  src = eyePosition + Vector(0.0f, 0.0f, -16.0f) + g_pGlobals->v_right * 16.0f;
  forward = eyePosition + Vector(0.0f, 0.0f, -16.0f) -
            g_pGlobals->v_right * 16.0f + normal * 24.0f;

  TraceLine(src, forward, TraceIgnore::Nothing, m_myself, &tr);

  // check if the trace hit something...
  if (tr.flFraction < 1.0f &&
      cstrncmp("func_door", STRING(tr.pHit->v.classname), 9))
    return true; // bot's body will hit something

  // now check below waist
  if ((pev->flags & FL_DUCKING)) {
    src = pev->origin + Vector(0.0f, 0.0f, -19.0f + 19.0f);
    forward = src + Vector(0.0f, 0.0f, 10.0f) + normal * 24.0f;

    TraceLine(src, forward, TraceIgnore::Nothing, m_myself, &tr);

    // check if the trace hit something...
    if (tr.flFraction < 1.0f &&
        cstrncmp("func_door", STRING(tr.pHit->v.classname), 9))
      return true; // bot's body will hit something

    src = pev->origin;
    forward = src + normal * 24.0f;

    TraceLine(src, forward, TraceIgnore::Nothing, m_myself, &tr);

    // check if the trace hit something...
    if (tr.flFraction < 1.0f &&
        cstrncmp("func_door", STRING(tr.pHit->v.classname), 9))
      return true; // bot's body will hit something
  } else {
    // trace from the left waist to the right forward waist pos
    src =
        pev->origin + Vector(0.0f, 0.0f, -17.0f) - g_pGlobals->v_right * 16.0f;
    forward = pev->origin + Vector(0.0f, 0.0f, -17.0f) +
              g_pGlobals->v_right * 16.0f + normal * 24.0f;

    // trace from the bot's waist straight forward...
    TraceLine(src, forward, TraceIgnore::Nothing, m_myself, &tr);

    // check if the trace hit something...
    if (tr.flFraction < 1.0f &&
        cstrncmp("func_door", STRING(tr.pHit->v.classname), 9))
      return true; // bot's body will hit something

    // trace from the left waist to the right forward waist pos
    src =
        pev->origin + Vector(0.0f, 0.0f, -17.0f) + g_pGlobals->v_right * 16.0f;
    forward = pev->origin + Vector(0.0f, 0.0f, -17.0f) -
              g_pGlobals->v_right * 16.0f + normal * 24.0f;

    TraceLine(src, forward, TraceIgnore::Nothing, m_myself, &tr);

    // check if the trace hit something...
    if (tr.flFraction < 1.0f &&
        cstrncmp("func_door", STRING(tr.pHit->v.classname), 9))
      return true; // bot's body will hit something
  }

  return false; // bot can move forward, return false
}

bool Bot::CanJumpUp(const Vector &normal) {
  // this function check if bot can jump over some obstacle
  TraceResult tr;

  // can't jump if not on ground and not on ladder/swimming
  if (!IsOnFloor() && (IsOnLadder() || !IsInWater()))
    return false;

  MakeVectors(Vector(0.0f, pev->angles.y, 0.0f));

  // check for normal jump height first...
  Vector src = pev->origin + Vector(0.0f, 0.0f, -36.0f + 45.0f);
  Vector dest = src + normal * 32.0f;

  // trace a line forward at maximum jump height...
  TraceLine(src, dest, TraceIgnore::Nothing, m_myself, &tr);

  if (tr.flFraction < 1.0f)
    goto CheckDuckJump;
  else {
    // now trace from jump height upward to check for obstructions...
    src = dest;
    dest.z = dest.z + 37.0f;

    TraceLine(src, dest, TraceIgnore::Nothing, m_myself, &tr);
    if (tr.flFraction < 1.0f)
      return false;
  }

  // now check same height to one side of the bot...
  src = pev->origin + g_pGlobals->v_right * 16.0f +
        Vector(0.0f, 0.0f, -36.0f + 45.0f);
  dest = src + normal * 32.0f;

  // trace a line forward at maximum jump height...
  TraceLine(src, dest, TraceIgnore::Nothing, m_myself, &tr);

  // if trace hit something, return false
  if (tr.flFraction < 1.0f)
    goto CheckDuckJump;

  // now trace from jump height upward to check for obstructions...
  src = dest;
  dest.z = dest.z + 37.0f;

  TraceLine(src, dest, TraceIgnore::Nothing, m_myself, &tr);

  // if trace hit something, return false
  if (tr.flFraction < 1.0f)
    return false;

  // now check same height on the other side of the bot...
  src = pev->origin + (-g_pGlobals->v_right * 16.0f) +
        Vector(0.0f, 0.0f, -36.0f + 45.0f);
  dest = src + normal * 32.0f;

  // trace a line forward at maximum jump height...
  TraceLine(src, dest, TraceIgnore::Nothing, m_myself, &tr);

  // if trace hit something, return false
  if (tr.flFraction < 1.0f)
    goto CheckDuckJump;

  // now trace from jump height upward to check for obstructions...
  src = dest;
  dest.z = dest.z + 37.0f;

  TraceLine(src, dest, TraceIgnore::Nothing, m_myself, &tr);

  // if trace hit something, return false
  return tr.flFraction >= 1.0f;

  // here we check if a duck jump would work...
CheckDuckJump:
  // use center of the body first... maximum duck jump height is 62, so check
  // one unit above that (63)
  src = pev->origin + Vector(0.0f, 0.0f, -36.0f + 63.0f);
  dest = src + normal * 32.0f;

  // trace a line forward at maximum jump height...
  TraceLine(src, dest, TraceIgnore::Nothing, m_myself, &tr);

  if (tr.flFraction < 1.0f)
    return false;
  else {
    // now trace from jump height upward to check for obstructions...
    src = dest;
    dest.z = dest.z + 37.0f;

    TraceLine(src, dest, TraceIgnore::Nothing, m_myself, &tr);

    // if trace hit something, check duckjump
    if (tr.flFraction < 1.0f)
      return false;
  }

  // now check same height to one side of the bot...
  src = pev->origin + g_pGlobals->v_right * 16.0f +
        Vector(0.0f, 0.0f, -36.0f + 63.0f);
  dest = src + normal * 32.0f;

  // trace a line forward at maximum jump height...
  TraceLine(src, dest, TraceIgnore::Nothing, m_myself, &tr);

  // if trace hit something, return false
  if (tr.flFraction < 1.0f)
    return false;

  // now trace from jump height upward to check for obstructions...
  src = dest;
  dest.z = dest.z + 37.0f;

  TraceLine(src, dest, TraceIgnore::Nothing, m_myself, &tr);

  // if trace hit something, return false
  if (tr.flFraction < 1.0f)
    return false;

  // now check same height on the other side of the bot...
  src = pev->origin + (-g_pGlobals->v_right * 16.0f) +
        Vector(0.0f, 0.0f, -36.0f + 63.0f);
  dest = src + normal * 32.0f;

  // trace a line forward at maximum jump height...
  TraceLine(src, dest, TraceIgnore::Nothing, m_myself, &tr);

  // if trace hit something, return false
  if (tr.flFraction < 1.0f)
    return false;

  // now trace from jump height upward to check for obstructions...
  src = dest;
  dest.z = dest.z + 37.0f;

  TraceLine(src, dest, TraceIgnore::Nothing, m_myself, &tr);

  // if trace hit something, return false
  return tr.flFraction >= 1.0f;
}

// this function check if bot can duck under obstacle
bool Bot::CanDuckUnder(const Vector &normal) {
  TraceResult tr{};
  Vector baseHeight;

  // use center of the body first...
  if (pev->flags & FL_DUCKING)
    baseHeight = pev->origin + Vector(0.0f, 0.0f, -17.0f);
  else
    baseHeight = pev->origin;

  Vector src = baseHeight;
  Vector dest = src + normal * 32.0f;

  // trace a line forward at duck height...
  TraceLine(src, dest, TraceIgnore::Nothing, pev->pContainingEntity, &tr);

  // if trace hit something, return false
  if (tr.flFraction < 1.0f)
    return false;

  MakeVectors(Vector(0.0f, pev->angles.y, 0.0f));

  // now check same height to one side of the bot...
  src = baseHeight + g_pGlobals->v_right * 16.0f;
  dest = src + normal * 32.0f;

  // trace a line forward at duck height...
  TraceLine(src, dest, TraceIgnore::Nothing, pev->pContainingEntity, &tr);

  // if trace hit something, return false
  if (tr.flFraction < 1.0f)
    return false;

  // now check same height on the other side of the bot...
  src = baseHeight + (-g_pGlobals->v_right * 16.0f);
  dest = src + normal * 32.0f;

  // trace a line forward at duck height...
  TraceLine(src, dest, TraceIgnore::Nothing, pev->pContainingEntity, &tr);

  // if trace hit something, return false
  return tr.flFraction >= 1.0f;
}

bool Bot::CheckWallOnForward(void) {
  TraceResult tr;
  MakeVectors(pev->angles);

  // do a trace to the forward...
  TraceLine(pev->origin, pev->origin + g_pGlobals->v_forward * 54.0f,
            TraceIgnore::Nothing, m_myself, &tr);

  // check if the trace hit something...
  if (tr.flFraction < 1.0f)
    return true;
  else {
    TraceLine(tr.vecEndPos, tr.vecEndPos - g_pGlobals->v_up * 54.0f,
              TraceIgnore::Nothing, m_myself, &tr);

    // we don't want to fall
    if (tr.flFraction >= 1.0f)
      return true;
  }

  return false;
}

bool Bot::CheckWallOnBehind(void) {
  TraceResult tr;
  MakeVectors(pev->angles);

  // do a trace to the behind...
  TraceLine(pev->origin, pev->origin - g_pGlobals->v_forward * 54.0f,
            TraceIgnore::Nothing, m_myself, &tr);

  // check if the trace hit something...
  if (tr.flFraction < 1.0f)
    return true;
  else {
    TraceLine(tr.vecEndPos, tr.vecEndPos - g_pGlobals->v_up * 54.0f,
              TraceIgnore::Nothing, m_myself, &tr);

    // we don't want to fall
    if (tr.flFraction >= 1.0f)
      return true;
  }

  return false;
}

bool Bot::CheckWallOnLeft(void) {
  TraceResult tr;
  MakeVectors(pev->angles);

  // do a trace to the left...
  TraceLine(pev->origin, pev->origin - g_pGlobals->v_right * 54.0f,
            TraceIgnore::Nothing, m_myself, &tr);

  // check if the trace hit something...
  if (tr.flFraction < 1.0f)
    return true;
  else {
    TraceLine(tr.vecEndPos, tr.vecEndPos - g_pGlobals->v_up * 54.0f,
              TraceIgnore::Nothing, m_myself, &tr);

    // we don't want to fall
    if (tr.flFraction >= 1.0f)
      return true;
  }

  return false;
}

bool Bot::CheckWallOnRight(void) {
  TraceResult tr;
  MakeVectors(pev->angles);

  // do a trace to the right...
  TraceLine(pev->origin, pev->origin + g_pGlobals->v_right * 54.0f,
            TraceIgnore::Nothing, m_myself, &tr);

  // check if the trace hit something...
  if (tr.flFraction < 1.0f)
    return true;
  else {
    TraceLine(tr.vecEndPos, tr.vecEndPos - g_pGlobals->v_up * 54.0f,
              TraceIgnore::Nothing, m_myself, &tr);

    // we don't want to fall
    if (tr.flFraction >= 1.0f)
      return true;
  }

  return false;
}

// this function eturns if given location would hurt Bot with falling damage
bool Bot::IsDeadlyDrop(const Vector &targetOriginPos) {
  int tries = 0;
  const Vector botPos = pev->origin;
  TraceResult tr;

  const Vector move((targetOriginPos - botPos).ToYaw(), 0.0f, 0.0f);
  MakeVectors(move);

  const Vector direction =
      (targetOriginPos - botPos).Normalize(); // 1 unit long
  Vector check = botPos;
  Vector down = botPos;

  down.z = down.z - 1000.0f; // straight down 1000 units

  TraceHull(check, down, TraceIgnore::Monsters, head_hull, m_myself, &tr);
  if (tr.flFraction > 0.036f) // We're not on ground anymore?
    tr.flFraction = 0.036f;

  float height;
  float lastHeight = tr.flFraction * 1000.0f; // height from ground

  float distance =
      (targetOriginPos - check).GetLengthSquared(); // distance from goal
  while (distance > squaredf(16.0f) && tries < 1000) {
    check = check + direction * 16.0f; // move 16 units closer to the goal...

    down = check;
    down.z = down.z - 1000.0f; // straight down 1000 units

    TraceHull(check, down, TraceIgnore::Monsters, head_hull, m_myself, &tr);
    if (tr.fStartSolid) // wall blocking?
      return false;

    height = tr.flFraction * 1000.0f; // height from ground
    if (lastHeight < height - 100.0f) // drops more than 100 units?
      return true;

    lastHeight = height;
    distance =
        (targetOriginPos - check).GetLengthSquared(); // distance from goal
    tries++;
  }

  return false;
}

void Bot::LookAt(const Vector &origin, const Vector &velocity) {
  m_updateLooking = true;
  m_lookAt = origin;
  m_lookVelocity = velocity;
}

void Bot::SetStrafeSpeed(const Vector &moveDir, const float strafeSpeed) {
  MakeVectors(pev->angles);
  if (CheckWallOnBehind()) {
    if (CheckWallOnRight())
      m_tempstrafeSpeed = -strafeSpeed;
    else if (CheckWallOnLeft())
      m_tempstrafeSpeed = strafeSpeed;

    m_strafeSpeed = m_tempstrafeSpeed;
    if ((m_isStuck || pev->speed >= pev->maxspeed) && IsOnFloor() &&
        !IsOnLadder() && m_jumpTime + 5.0f < engine->GetTime() &&
        CanJumpUp(moveDir)) {
      m_buttons |= IN_JUMP;
      TryStartDoubleJump(this, m_waypoint.origin);
    }
  } else if (((moveDir - pev->origin).Normalize2D() |
              g_pGlobals->v_forward.SkipZ()) > 0.0f &&
             !CheckWallOnRight())
    m_strafeSpeed = strafeSpeed;
  else if (!CheckWallOnLeft())
    m_strafeSpeed = -strafeSpeed;
}

void Bot::SetStrafeSpeedNoCost(const Vector &moveDir, const float strafeSpeed) {
  MakeVectors(pev->angles);
  if (((moveDir - pev->origin).Normalize2D() | g_pGlobals->v_forward.SkipZ()) >
      0.0f)
    m_strafeSpeed = strafeSpeed;
  else
    m_strafeSpeed = -strafeSpeed;
}

bool Bot::IsWaypointOccupied(const int16_t index) {
  if (ebot_has_semiclip.GetBool())
    return false;

  if (pev->solid == SOLID_NOT)
    return false;

  Path *pointer;
  Bot *bot;
  for (const Clients &client : g_clients) {
    if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE) ||
        client.team != m_team || client.ent == m_myself)
      continue;

    bot = g_botManager->GetBot(client.index);
    if (bot && bot->m_isAlive) {
      if (bot->m_currentWaypointIndex == index)
        return true;

      if (bot->m_prevWptIndex[0] == index)
        return true;

      if (!bot->m_navNode.IsEmpty() && bot->m_navNode.Last() == index)
        return true;
    } else {
      pointer = g_waypoint->GetPath(index);
      if (pointer &&
          ((client.ent->v.origin + client.ent->v.velocity * m_frameInterval) -
           pointer->origin)
                  .GetLengthSquared() < squaredi(pointer->radius + 54))
        return true;
    }
  }

  return false;
}

// this function tries to find nearest to current bot button, and returns
// pointer to it's entity, also here must be specified the target, that button
// must open
edict_t *Bot::FindNearestButton(const char *className) {
  if (IsNullString(className))
    return nullptr;

  float nearestDistance = 65355.0f;
  edict_t *searchEntity = nullptr;
  edict_t *foundEntity = nullptr;
  float distance;

  // find the nearest button which can open our target
  while (!FNullEnt(searchEntity =
                       FIND_ENTITY_BY_TARGET(searchEntity, className))) {
    distance = (pev->origin - GetEntityOrigin(searchEntity)).GetLengthSquared();
    if (distance < nearestDistance) {
      nearestDistance = distance;
      foundEntity = searchEntity;
    }
  }

  return foundEntity;
}
