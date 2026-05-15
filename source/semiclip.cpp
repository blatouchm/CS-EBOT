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

#include "../include/core.h"

constexpr int kMaxSemiclipClients = 32;
constexpr int kMaxPhysents = 600;
constexpr float kCrouchHeightDelta = 35.9f;
constexpr float kZombieUnstuckPairTime = 0.25f;

ConVar ebot_semiclip("ebot_semiclip", "0");
ConVar ebot_semiclip_team("ebot_semiclip_team", "3");

extern ConVar ebot_has_semiclip;
extern ConVar ebot_debug;

struct entity_state_s
{
	int entityType;
	int number;
	float msg_time;
	int messagenum;
	Vector origin;
	Vector angles;
	int modelindex;
	int sequence;
	float frame;
	int colormap;
	short skin;
	short solid;
	int effects;
	float scale;
	uint8_t eflags;
	int rendermode;
	int renderamt;
};

struct physent_s
{
	char name[32];
	int player;
	Vector origin;
	void *model;
	void *studiomodel;
	Vector mins;
	Vector maxs;
	int info;
	Vector angles;
	int solid;
	int skin;
	int rendermode;
	float frame;
	int sequence;
	uint8_t controller[4];
	uint8_t blending[2];
	int movetype;
	int takedamage;
	int blooddecal;
	int team;
	int classnumber;
	int iuser1;
	int iuser2;
	int iuser3;
	int iuser4;
	float fuser1;
	float fuser2;
	float fuser3;
	float fuser4;
	Vector vuser1;
	Vector vuser2;
	Vector vuser3;
	Vector vuser4;
};

struct playermove_s
{
	int player_index;
	qboolean server;
	qboolean multiplayer;
	float time;
	float frametime;
	Vector forward;
	Vector right;
	Vector up;
	Vector origin;
	Vector angles;
	Vector oldangles;
	Vector velocity;
	Vector movedir;
	Vector basevelocity;
	Vector view_ofs;
	float flDuckTime;
	qboolean bInDuck;
	int flTimeStepSound;
	int iStepLeft;
	float flFallVelocity;
	Vector punchangle;
	float flSwimTime;
	float flNextPrimaryAttack;
	int effects;
	int flags;
	int usehull;
	float gravity;
	float friction;
	int oldbuttons;
	float waterjumptime;
	qboolean dead;
	int deadflag;
	int spectator;
	int movetype;
	int onground;
	int waterlevel;
	int watertype;
	int oldwaterlevel;
	char sztexturename[256];
	char chtexturetype;
	float maxspeed;
	float clientmaxspeed;
	int iuser1;
	int iuser2;
	int iuser3;
	int iuser4;
	float fuser1;
	float fuser2;
	float fuser3;
	float fuser4;
	Vector vuser1;
	Vector vuser2;
	Vector vuser3;
	Vector vuser4;
	int numphysent;
	physent_s physents[kMaxPhysents];
};

struct SemiclipPlayer
{
	bool active[kMaxSemiclipClients + 1];
	bool boostSolid[kMaxSemiclipClients + 1];
	float distanceSq[kMaxSemiclipClients + 1];
};

static SemiclipPlayer g_semiclip[kMaxSemiclipClients + 1];
static float g_zombieUnstuckPairUntil[kMaxSemiclipClients + 1][kMaxSemiclipClients + 1];
static DLL_FUNCTIONS *g_semiclipPreHookTable = nullptr;
static DLL_FUNCTIONS *g_semiclipPostHookTable = nullptr;
static enginefuncs_t *g_semiclipEnginePostHookTable = nullptr;
static bool g_semiclipHooksEnabled = true;


void SemiclipTraceLine_Post(const float *v1, const float *v2, int fNoMonsters, edict_t *pentToSkip, TraceResult *ptr);

void SemiclipSetPreHookTable(DLL_FUNCTIONS *functionTable)
{
	g_semiclipPreHookTable = functionTable;

	if (g_semiclipHooksEnabled && g_semiclipPreHookTable)
		g_semiclipPreHookTable->pfnPM_Move = SemiclipPMMove;
}

void SemiclipSetPostHookTable(DLL_FUNCTIONS *functionTable)
{
	g_semiclipPostHookTable = functionTable;

	if (g_semiclipHooksEnabled && g_semiclipPostHookTable)
		g_semiclipPostHookTable->pfnAddToFullPack = SemiclipAddToFullPack_Post;
}

void SemiclipSetEnginePostHookTable(enginefuncs_t *functionTable)
{
	g_semiclipEnginePostHookTable = functionTable;

	if (g_semiclipHooksEnabled && g_semiclipEnginePostHookTable)
		g_semiclipEnginePostHookTable->pfnTraceLine = SemiclipTraceLine_Post;
}

void SemiclipUpdateHooks(void)
{
	g_semiclipHooksEnabled = !ebot_semiclip.m_eptr || ebot_semiclip.GetBool();

	if (g_semiclipPreHookTable)
		g_semiclipPreHookTable->pfnPM_Move = g_semiclipHooksEnabled ? SemiclipPMMove : nullptr;

	if (g_semiclipPostHookTable)
		g_semiclipPostHookTable->pfnAddToFullPack = g_semiclipHooksEnabled ? SemiclipAddToFullPack_Post : nullptr;

	if (g_semiclipEnginePostHookTable)
		g_semiclipEnginePostHookTable->pfnTraceLine = g_semiclipHooksEnabled ? SemiclipTraceLine_Post : nullptr;
}

static bool IsSemiclipTeamAllowed(const int hostTeam, const int otherTeam)
{
	switch (cclamp(ebot_semiclip_team.GetInt(), 0, 3))
	{
	case 0:
		return true;

	case 1:
		return hostTeam == Team::Terrorist && otherTeam == Team::Terrorist;

	case 2:
		return hostTeam == Team::Counter && otherTeam == Team::Counter;

	case 3:
	default:
		return hostTeam == otherTeam;
	}
}

static bool IsValidPlayerIndex(const int index)
{
	return index > 0 && index <= g_maxClients;
}

static bool ArePlayerBoundsTouching(edict_t *first, edict_t *second)
{
	constexpr float touchEpsilon = 1.0f;

	return first->v.absmin.x <= second->v.absmax.x + touchEpsilon &&
		first->v.absmax.x + touchEpsilon >= second->v.absmin.x &&
		first->v.absmin.y <= second->v.absmax.y + touchEpsilon &&
		first->v.absmax.y + touchEpsilon >= second->v.absmin.y &&
		first->v.absmin.z <= second->v.absmax.z + touchEpsilon &&
		first->v.absmax.z + touchEpsilon >= second->v.absmin.z;
}

static void ActivateZombieUnstuckPair(const int firstIndex, const int secondIndex, const float time)
{
	const float until = time + kZombieUnstuckPairTime;
	g_zombieUnstuckPairUntil[firstIndex][secondIndex] = until;
	g_zombieUnstuckPairUntil[secondIndex][firstIndex] = until;
}

static bool HasZombieUnstuckPair(const int firstIndex, const int secondIndex)
{
	return g_zombieUnstuckPairUntil[firstIndex][secondIndex] > engine->GetTime();
}

static bool HasTouchingSemiclipPlayer(edict_t *player, const int playerIndex)
{
	for (int otherIndex = 1; otherIndex <= g_maxClients; ++otherIndex)
	{
		if (!g_semiclip[playerIndex].active[otherIndex] || otherIndex == playerIndex)
			continue;

		edict_t *other = INDEXENT(otherIndex);
		if (!IsValidPlayer(other) || !IsAlive(other))
			continue;

		if (ArePlayerBoundsTouching(player, other))
			return true;
	}

	return false;
}

/// <summary>
/// check semiclip conditions before disabling it beetween players who were previously in semiclip
/// (due to a bug that causes the player to fly through the ceiling)
/// </summary>
/// <param name="host"></param>
/// <param name="hostIndex"></param>
/// <param name="hostTeam"></param>
static void CheckChangedSemiclipConditions(edict_t *host, const int hostIndex, const int hostTeam)
{
	const float time = engine->GetTime();
	for (int otherIndex = 1; otherIndex <= g_maxClients; ++otherIndex)
	{
		if (!g_semiclip[hostIndex].active[otherIndex] || otherIndex == hostIndex)
			continue;

		if (HasZombieUnstuckPair(hostIndex, otherIndex))
			continue;

		edict_t *other = INDEXENT(otherIndex);
		if (!IsValidPlayer(other) || !IsAlive(other))
			continue;

		if (IsSemiclipTeamAllowed(hostTeam, GetTeam(other)))
			continue;

		if (!ArePlayerBoundsTouching(host, other))
			continue;

		ActivateZombieUnstuckPair(hostIndex, otherIndex, time);
	}
}

static bool IsKnifeAttackButtonActive(const Bot *bot)
{
	const int attackButtons = IN_ATTACK | IN_ATTACK2;
	return (bot->m_buttons & attackButtons) || (bot->m_oldButtons & attackButtons);
}

static bool IsKnifeWeaponEquipped(edict_t *player, const Bot *bot)
{
	if (bot)
		return bot->m_currentWeapon == Weapon::Knife;

	const int playerIndex = ENTINDEX(player);
	return playerIndex > 0 && playerIndex < 33 &&
		g_playerCurrentWeapon[playerIndex] == Weapon::Knife;
}

static bool IntersectTraceBounds(const Vector &start, const Vector &end, const Vector &mins, const Vector &maxs, float *fraction, Vector *normal)
{
	const Vector delta = end - start;
	float enterFraction = 0.0f;
	float exitFraction = 1.0f;
	Vector enterNormal = nullvec;

	for (int axis = 0; axis < 3; ++axis)
	{
		const float startValue = axis == 0 ? start.x : axis == 1 ? start.y : start.z;
		const float deltaValue = axis == 0 ? delta.x : axis == 1 ? delta.y : delta.z;
		const float minValue = axis == 0 ? mins.x : axis == 1 ? mins.y : mins.z;
		const float maxValue = axis == 0 ? maxs.x : axis == 1 ? maxs.y : maxs.z;

		if (cabsf(deltaValue) < 0.0001f)
		{
			if (startValue < minValue || startValue > maxValue)
				return false;

			continue;
		}

		float axisEnter;
		float axisExit;
		Vector axisNormal = nullvec;

		if (deltaValue > 0.0f)
		{
			axisEnter = (minValue - startValue) / deltaValue;
			axisExit = (maxValue - startValue) / deltaValue;
			if (axis == 0)
				axisNormal.x = -1.0f;
			else if (axis == 1)
				axisNormal.y = -1.0f;
			else
				axisNormal.z = -1.0f;
		}
		else
		{
			axisEnter = (maxValue - startValue) / deltaValue;
			axisExit = (minValue - startValue) / deltaValue;
			if (axis == 0)
				axisNormal.x = 1.0f;
			else if (axis == 1)
				axisNormal.y = 1.0f;
			else
				axisNormal.z = 1.0f;
		}

		if (axisEnter > enterFraction)
		{
			enterFraction = axisEnter;
			enterNormal = axisNormal;
		}

		if (axisExit < exitFraction)
			exitFraction = axisExit;

		if (enterFraction > exitFraction)
			return false;
	}

	if (exitFraction < 0.0f || enterFraction > 1.0f)
		return false;

	if (fraction)
		*fraction = cclampf(enterFraction, 0.0f, 1.0f);

	if (normal)
		*normal = enterNormal;

	return true;
}

static bool BuildKnifeTraceEntityResult(edict_t *target, const Vector &start, const Vector &end, TraceResult *trace, float *fraction = nullptr)
{
	if (FNullEnt(target))
		return false;

	Vector mins = target->v.absmin;
	Vector maxs = target->v.absmax;

	if (mins.IsNull() && maxs.IsNull())
	{
		const Vector center = GetEntityOrigin(target);
		mins = center - Vector(8.0f, 8.0f, 8.0f);
		maxs = center + Vector(8.0f, 8.0f, 8.0f);
	}

	constexpr float targetExpand = 4.0f;
	mins = mins - Vector(targetExpand, targetExpand, targetExpand);
	maxs = maxs + Vector(targetExpand, targetExpand, targetExpand);

	float hitFraction = 1.0f;
	Vector normal = nullvec;
	if (!IntersectTraceBounds(start, end, mins, maxs, &hitFraction, &normal))
		return false;

	trace->fAllSolid = false;
	trace->fStartSolid = false;
	trace->flFraction = hitFraction;
	trace->vecEndPos = start + (end - start) * hitFraction;
	trace->flPlaneDist = 0.0f;
	trace->vecPlaneNormal = normal;
	trace->pHit = target;
	trace->iHitgroup = 0;

	if (fraction)
		*fraction = hitFraction;

	return true;
}

static bool IsKnifeTraceBlockedBySemiclipPlayer(edict_t *attacker, const int attackerIndex, edict_t *hit)
{
	if (FNullEnt(attacker) || FNullEnt(hit) || attacker == hit || !IsValidPlayer(hit) || !IsAlive(hit))
		return false;

	const int hitIndex = ENTINDEX(hit);
	return IsValidPlayerIndex(attackerIndex) && IsValidPlayerIndex(hitIndex) &&
		g_semiclip[attackerIndex].active[hitIndex] && GetTeam(attacker) == GetTeam(hit);
}

static bool IsKnifeTraceReplacementPlayer(edict_t *attacker, edict_t *target, edict_t *blocker)
{
	return !FNullEnt(target) && target != attacker && target != blocker &&
		IsValidPlayer(target) && IsAlive(target) && GetTeam(target) != GetTeam(attacker);
}

static bool FindKnifeTraceBlockingSemiclipPlayer(edict_t *attacker, const int attackerIndex, const Vector &start, const Vector &end,
	const float maxFraction, edict_t **blocker, float *blockerFraction)
{
	constexpr float blockerEpsilon = 0.001f;

	if (FNullEnt(attacker) || !blocker || !blockerFraction || !IsValidPlayerIndex(attackerIndex))
		return false;

	edict_t *bestBlocker = nullptr;
	float bestBlockerFraction = cclampf(maxFraction, 0.0f, 1.0f);
	if (bestBlockerFraction <= 0.0f)
		return false;

	for (int targetIndex = 1; targetIndex <= g_maxClients; ++targetIndex)
	{
		edict_t *target = INDEXENT(targetIndex);
		if (!IsKnifeTraceBlockedBySemiclipPlayer(attacker, attackerIndex, target))
			continue;

		TraceResult candidateTrace{};
		float candidateFraction = 1.0f;
		if (!BuildKnifeTraceEntityResult(target, start, end, &candidateTrace, &candidateFraction))
			continue;

		if (candidateFraction + blockerEpsilon >= bestBlockerFraction)
			continue;

		bestBlocker = target;
		bestBlockerFraction = candidateFraction;
	}

	if (FNullEnt(bestBlocker))
		return false;

	*blocker = bestBlocker;
	*blockerFraction = bestBlockerFraction;
	return true;
}

static bool IsKnifeTraceReplacementEntity(edict_t *attacker, edict_t *target, edict_t *blocker)
{
	if (FNullEnt(target) || target == attacker || target == blocker)
		return false;

	if (IsValidPlayer(target))
		return IsKnifeTraceReplacementPlayer(attacker, target, blocker);

	return target->v.takedamage != DAMAGE_NO || IsBreakable(target);
}

static bool FindKnifeTraceReplacementTarget(edict_t *attacker, edict_t *blocker, const Vector &start, const Vector &end,
	const float blockerFraction, const float maxFraction, TraceResult *trace)
{
	constexpr float blockerEpsilon = 0.001f;

	edict_t *bestTarget = nullptr;
	float bestFraction = cclampf(maxFraction, 0.0f, 1.0f);
	if (bestFraction <= blockerFraction + blockerEpsilon)
		return false;

	for (int targetIndex = 1; targetIndex <= g_maxClients; ++targetIndex)
	{
		edict_t *target = INDEXENT(targetIndex);
		if (!IsKnifeTraceReplacementPlayer(attacker, target, blocker))
			continue;

		TraceResult candidateTrace{};
		float candidateFraction = 1.0f;
		if (!BuildKnifeTraceEntityResult(target, start, end, &candidateTrace, &candidateFraction))
			continue;

		if (candidateFraction <= blockerFraction + blockerEpsilon || candidateFraction >= bestFraction)
			continue;

		bestTarget = target;
		bestFraction = candidateFraction;
		*trace = candidateTrace;
	}

	const Vector center = (start + end) * 0.5f;
	const float radius = (end - start).GetLength() * 0.5f + 32.0f;
	edict_t *search = nullptr;
	while (!FNullEnt(search = FIND_ENTITY_IN_SPHERE(search, center, radius)))
	{
		if (!IsKnifeTraceReplacementEntity(attacker, search, blocker))
			continue;

		const int index = ENTINDEX(search);
		if (index > 0 && index <= g_maxClients)
			continue;

		TraceResult candidateTrace{};
		float candidateFraction = 1.0f;
		if (!BuildKnifeTraceEntityResult(search, start, end, &candidateTrace, &candidateFraction))
			continue;

		if (candidateFraction <= blockerFraction + blockerEpsilon || candidateFraction >= bestFraction)
			continue;

		bestTarget = search;
		bestFraction = candidateFraction;
		*trace = candidateTrace;
	}

	return !FNullEnt(bestTarget);
}

static bool IsDoubleJumpBot(edict_t *ent)
{
	Bot *bot = g_botManager->GetBot(ent);
	return bot && bot->m_isAlive && (bot->m_waypoint.flags & WAYPOINT_DJUMP);
}

static bool ShouldKeepSolidForDoubleJump(edict_t *host, edict_t *other, const int hostIndex, const int otherIndex)
{
	if (!IsDoubleJumpBot(host) && !IsDoubleJumpBot(other))
		return false;

	const float zDelta = cabsf(host->v.origin.z - other->v.origin.z);
	const bool bothDucking = (host->v.buttons & IN_DUCK || host->v.flags & FL_DUCKING) &&
		(other->v.buttons & IN_DUCK || other->v.flags & FL_DUCKING);

	if (zDelta < kCrouchHeightDelta)
	{
		g_semiclip[hostIndex].boostSolid[otherIndex] = false;
		g_semiclip[otherIndex].boostSolid[hostIndex] = false;
		return false;
	}

	if (bothDucking)
	{
		g_semiclip[hostIndex].boostSolid[otherIndex] = true;
		g_semiclip[otherIndex].boostSolid[hostIndex] = true;
		return true;
	}

	return host->v.groundentity == other || other->v.groundentity == host ||
		g_semiclip[hostIndex].boostSolid[otherIndex] ||
		g_semiclip[otherIndex].boostSolid[hostIndex];
}

static bool ShouldSemiclipPlayer(edict_t *host, const int hostIndex, const int otherIndex, const int hostTeam, const float maxDistanceSq)
{
	if (otherIndex <= 0 || otherIndex > g_maxClients || otherIndex == hostIndex)
		return false;

	edict_t* other = INDEXENT(otherIndex);
	if (!IsValidPlayer(other) || !IsAlive(other))
		return false;

	const bool zombieUnstuckPair = HasZombieUnstuckPair(hostIndex, otherIndex);

	const int otherTeam = GetTeam(other);

	if (!zombieUnstuckPair && !IsSemiclipTeamAllowed(hostTeam, otherTeam))
		return false;

	const float distanceSq = (host->v.origin - other->v.origin).GetLengthSquared2D();
	g_semiclip[hostIndex].distanceSq[otherIndex] = distanceSq;
	if (distanceSq > maxDistanceSq)
		return false;

	if (!zombieUnstuckPair && ShouldKeepSolidForDoubleJump(host, other, hostIndex, otherIndex))
		return false;

	return true;
}

void SemiclipReset(void)
{
	cmemset(g_semiclip, 0, sizeof(g_semiclip));
	cmemset(g_zombieUnstuckPairUntil, 0, sizeof(g_zombieUnstuckPairUntil));
	SemiclipUpdateHooks();

	if (g_semiclipHooksEnabled)
		ebot_has_semiclip.SetInt(1);
}

void SemiclipPMMove(playermove_s *pmove, qboolean /*server*/)
{
	if (!g_semiclipHooksEnabled || !pmove || pmove->spectator || pmove->dead || pmove->deadflag != DEAD_NO)
		RETURN_META(MRES_IGNORED);

	const int hostIndex = pmove->player_index + 1;
	if (hostIndex <= 0 || hostIndex > g_maxClients)
		RETURN_META(MRES_IGNORED);

	edict_t* host = INDEXENT(hostIndex);
	if (!IsValidPlayer(host) || !IsAlive(host))
		RETURN_META(MRES_IGNORED);

	const int hostTeam = GetTeam(host);
	CheckChangedSemiclipConditions(host, hostIndex, hostTeam);

	cmemset(&g_semiclip[hostIndex].active, 0, sizeof(g_semiclip[hostIndex].active));

	const float maxDistance = 64.0f;
	const float maxDistanceSq = maxDistance * maxDistance;
	bool hasAnySkip = false;
	int writeIndex = 0;

	for (int readIndex = 0; readIndex < pmove->numphysent && readIndex < kMaxPhysents; ++readIndex)
	{
		const int otherIndex = pmove->physents[readIndex].player;
		if (ShouldSemiclipPlayer(host, hostIndex, otherIndex, hostTeam, maxDistanceSq))
		{
			g_semiclip[hostIndex].active[otherIndex] = true;
			hasAnySkip = true;
			continue;
		}

		if (writeIndex != readIndex)
			pmove->physents[writeIndex] = pmove->physents[readIndex];

		++writeIndex;
	}

	if (!hasAnySkip)
		RETURN_META(MRES_IGNORED);

	pmove->numphysent = writeIndex;
	RETURN_META(MRES_IGNORED);
}

int SemiclipAddToFullPack_Post(entity_state_s *state, int e, edict_t *ent, edict_t *host, int /*hostflags*/, int player, uint8_t * /*pSet*/)
{
	if (!g_semiclipHooksEnabled || !state || !player || FNullEnt(ent) || FNullEnt(host) || ent == host)
		RETURN_META_VALUE(MRES_IGNORED, 0);

	const int hostIndex = ENTINDEX(host);
	const int entIndex = e > 0 ? e : ENTINDEX(ent);

	if (hostIndex <= 0 || hostIndex > g_maxClients || entIndex <= 0 || entIndex > g_maxClients)
		RETURN_META_VALUE(MRES_IGNORED, 0);

	if (g_semiclip[hostIndex].active[entIndex])
	{
		state->solid = SOLID_NOT;

		state->rendermode = kRenderTransTexture;

		if(g_semiclip[hostIndex].distanceSq[entIndex] > 24.0)
			state->renderamt = 130;
		else
			state->renderamt = 0;
	}

	RETURN_META_VALUE(MRES_IGNORED, 0);
}

/// <summary>
/// Fixes knife attacks for zombies during semiclip when a attacking player is inside another player.
/// For humans, this is not that critical, because most weapons can shoot through entities.
/// </summary>
void SemiclipTraceLine_Post(const float* v1, const float* v2, int /*fNoMonsters*/, edict_t* pentToSkip, TraceResult* ptr)
{
	if (!g_semiclipHooksEnabled || !ptr || FNullEnt(pentToSkip) || !IsValidPlayer(pentToSkip) || !IsZombieEntity(pentToSkip) || !IsAlive(pentToSkip))
		RETURN_META(MRES_IGNORED);

	Bot *bot = g_botManager->GetBot(pentToSkip);
	if (bot && !bot->m_isAlive)
		RETURN_META(MRES_IGNORED);

	if (!IsKnifeWeaponEquipped(pentToSkip, bot))
		RETURN_META(MRES_IGNORED);

	const int attackButtons = IN_ATTACK | IN_ATTACK2;

	if (bot)
	{
		if (!(bot->m_buttons & attackButtons) && !(bot->m_oldButtons & attackButtons))
			RETURN_META(MRES_IGNORED);
	}
	else if (!(pentToSkip->v.buttons & attackButtons) && !(pentToSkip->v.oldbuttons & attackButtons))
		RETURN_META(MRES_IGNORED);

	const Vector start(v1[0], v1[1], v1[2]);
	const Vector end(v2[0], v2[1], v2[2]);
	const Vector eyePosition = pentToSkip->v.origin + pentToSkip->v.view_ofs;
	if ((start - eyePosition).GetLengthSquared() > squaredf(64.0f) &&
		(start - pentToSkip->v.origin).GetLengthSquared() > squaredf(64.0f))
		RETURN_META(MRES_IGNORED);

	const float traceLength = (end - start).GetLength();
	if (traceLength < 8.0f || traceLength > 96.0f)
		RETURN_META(MRES_IGNORED);

	const int attackerIndex = ENTINDEX(pentToSkip);
	if (!IsValidPlayerIndex(attackerIndex) || !HasTouchingSemiclipPlayer(pentToSkip, attackerIndex))
		RETURN_META(MRES_IGNORED);

	edict_t *blockedTeammate = nullptr;
	float blockedFraction = ptr->flFraction;
	float replacementMaxFraction = 1.0f;
	bool redirectedFromMiss = false;

	if (ptr->flFraction < 1.0f && !FNullEnt(ptr->pHit) &&
		IsKnifeTraceBlockedBySemiclipPlayer(pentToSkip, attackerIndex, ptr->pHit))
	{
		blockedTeammate = ptr->pHit;
	}
	else
	{
		if (ptr->flFraction < 1.0f && !FNullEnt(ptr->pHit) &&
			IsKnifeTraceReplacementEntity(pentToSkip, ptr->pHit, nullptr))
			RETURN_META(MRES_IGNORED);

		replacementMaxFraction = cclampf(ptr->flFraction, 0.0f, 1.0f);
		if (!FindKnifeTraceBlockingSemiclipPlayer(pentToSkip, attackerIndex, start, end,
			replacementMaxFraction, &blockedTeammate, &blockedFraction))
			RETURN_META(MRES_IGNORED);

		redirectedFromMiss = true;
	}

	if (FNullEnt(blockedTeammate))
		RETURN_META(MRES_IGNORED);

	if (FindKnifeTraceReplacementTarget(pentToSkip, blockedTeammate, start, end, blockedFraction, replacementMaxFraction, ptr) &&
		ebot_debug.GetBool() && !FNullEnt(ptr->pHit) && !bot)
	{
		//only for debug
		char attackerName[32];
		char blockedName[32];
		char redirectedName[32];
		cstrncpy(attackerName, GetEntityName(pentToSkip), sizeof(attackerName));
		cstrncpy(blockedName, GetEntityName(blockedTeammate), sizeof(blockedName));
		cstrncpy(redirectedName, GetEntityName(ptr->pHit), sizeof(redirectedName));

		if (redirectedFromMiss)
		{
			ServerPrint("%s knife miss corrected through teammate %s to %s",
				attackerName, blockedName, redirectedName);
		}
		else
		{
			ServerPrint("%s knife trace redirected from teammate %s to %s",
				attackerName, blockedName, redirectedName);
		}
	}

	RETURN_META(MRES_IGNORED);
}
