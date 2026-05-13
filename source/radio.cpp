//
// Radio command handling for E-BOT.
//

#include "../include/core.h"
#include "../include/radio.h"

ConVar ebot_radio("ebot_radio", "1");
ConVar ebot_radio_follow_max_bots("ebot_radio_follow_max_bots", "3");
ConVar ebot_radio_follow_distance("ebot_radio_follow_distance", "700.0");
ConVar ebot_radio_follow_delay("ebot_radio_follow_delay", "2.0");

static int s_radioMenu[32]{0};
static bool s_radioWasEnabled{true};

struct RadioFollowCandidate
{
	Bot* bot{nullptr};
	float distanceSq{0.0f};
};

struct PendingRadioFollow
{
	edict_t* player{nullptr};
	float time{0.0f};
};

static PendingRadioFollow s_pendingFollow[32]{};

static bool IsRadioSourceValid(edict_t* ent)
{
	if (g_isFakeCommand || FNullEnt(ent) || IsValidBot(ent) || !IsAlive(ent))
		return false;

	const int clientIndex = ENTINDEX(ent) - 1;
	const int maxClients = cmin(engine->GetMaxClients(), 32);
	if (clientIndex < 0 || clientIndex >= maxClients)
		return false;

	const int team = GetTeam(ent);
	return team == Team::Terrorist || team == Team::Counter;
}

static int16_t GetRadioFollowWaypoint(edict_t* player)
{
	if (FNullEnt(player) || !g_waypoint || g_numWaypoints <= 0)
		return -1;

	const int clientIndex = ENTINDEX(player) - 1;
	const int maxClients = cmin(engine->GetMaxClients(), 32);
	if (clientIndex >= 0 && clientIndex < maxClients &&
		IsValidWaypoint(g_clients[clientIndex].wp))
		return g_clients[clientIndex].wp;

	int16_t waypoint = g_waypoint->FindNearestToEnt(player->v.origin, 99999.0f, player);
	if (!IsValidWaypoint(waypoint))
		waypoint = g_waypoint->FindNearest(player->v.origin, 99999.0f);

	return waypoint;
}

static int CountRadioFollowers(edict_t* player)
{
	int followers = 0;
	for (Bot* const& bot : g_botManager->m_bots)
	{
		if (bot && bot->m_isAlive && bot->m_radioFollowUser == player)
			followers++;
	}

	return followers;
}

static void BotPlayRadioMessage(Bot* bot, const int radioMessage)
{
	if (!ebot_radio.GetBool() || !bot || FNullEnt(bot->m_myself) || !bot->m_isAlive)
		return;

	if (radioMessage > 20)
		FakeClientCommand(bot->m_myself, "radio3; menuselect %d", radioMessage - 20);
	else if (radioMessage > 10)
		FakeClientCommand(bot->m_myself, "radio2; menuselect %d", radioMessage - 10);
	else if (radioMessage > 0)
		FakeClientCommand(bot->m_myself, "radio1; menuselect %d", radioMessage);
}

static bool IsCleanRadioFollowWaypoint(const int16_t waypoint)
{
	if (!IsValidWaypoint(waypoint) || !g_waypoint)
		return false;

	const Path* const path = g_waypoint->GetPath(waypoint);
	if (!path)
		return false;

	return (path->flags & (WAYPOINT_LADDER | WAYPOINT_JUMP | WAYPOINT_DJUMP)) == 0;
}

static bool IsCleanRadioFollowConnection(const int16_t from, const int16_t to)
{
	if (!IsCleanRadioFollowWaypoint(from) || !IsCleanRadioFollowWaypoint(to))
		return false;

	const Path* const path = g_waypoint->GetPath(from);
	if (!path)
		return false;

	for (int16_t i = 0; i < Const_MaxPathIndex; i++)
	{
		if (path->index[i] != to)
			continue;

		return (path->connectionFlags[i] & (PATHFLAG_JUMP | PATHFLAG_DOUBLE)) == 0;
	}

	return false;
}

static int16_t GetRadioFollowWaypointFor(edict_t* ent)
{
	if (FNullEnt(ent) || !g_waypoint || g_numWaypoints <= 0)
		return -1;

	const int clientIndex = ENTINDEX(ent) - 1;
	const int maxClients = cmin(engine->GetMaxClients(), 32);
	if (clientIndex >= 0 && clientIndex < maxClients &&
		IsValidWaypoint(g_clients[clientIndex].wp))
		return g_clients[clientIndex].wp;

	return g_waypoint->FindNearestToEnt(ent->v.origin, 99999.0f, ent);
}

static bool HasShortCleanRadioFollowPath(Bot* bot, edict_t* player)
{
	if (!bot || FNullEnt(bot->m_myself) || FNullEnt(player) || !g_waypoint)
		return false;

	int16_t botWaypoint = bot->m_currentWaypointIndex;
	if (!IsValidWaypoint(botWaypoint))
		botWaypoint = GetRadioFollowWaypointFor(bot->m_myself);

	const int16_t playerWaypoint = GetRadioFollowWaypointFor(player);
	if (!IsCleanRadioFollowWaypoint(botWaypoint) ||
		!IsCleanRadioFollowWaypoint(playerWaypoint))
		return false;

	if (botWaypoint == playerWaypoint)
		return true;

	if (IsCleanRadioFollowConnection(botWaypoint, playerWaypoint))
		return true;

	const Path* const path = g_waypoint->GetPath(botWaypoint);
	if (!path)
		return false;

	for (int16_t i = 0; i < Const_MaxPathIndex; i++)
	{
		const int16_t middleWaypoint = path->index[i];
		if (!IsValidWaypoint(middleWaypoint))
			continue;

		if (IsCleanRadioFollowConnection(botWaypoint, middleWaypoint) &&
			IsCleanRadioFollowConnection(middleWaypoint, playerWaypoint))
			return true;
	}

	return false;
}

static bool BotHasVisibleHumanEnemyWithin(Bot* bot, const float distance)
{
	if (!bot || !bot->pev || !bot->m_isZombieBot)
		return false;

	const float distanceSq = squaredf(distance);
	for (const Clients& client : g_clients)
	{
		if (!(client.flags & CFLAG_USED) || !(client.flags & CFLAG_ALIVE))
			continue;

		if (FNullEnt(client.ent) || client.ent == bot->m_myself)
			continue;

		if (!IsAlive(client.ent) || IsZombieEntity(client.ent))
			continue;

		if ((client.ent->v.origin - bot->pev->origin).GetLengthSquared() > distanceSq)
			continue;

		if (!HasShortCleanRadioFollowPath(bot, client.ent))
			continue;

		if (bot->CheckVisibility(client.ent))
			return true;
	}

	return false;
}

static bool ZombieBotHasHumanTargetWithin(Bot* bot, const float distance)
{
	if (!bot || !bot->pev || !bot->m_isZombieBot || FNullEnt(bot->m_nearestEnemy))
		return false;

	if (!IsAlive(bot->m_nearestEnemy) || IsZombieEntity(bot->m_nearestEnemy))
		return false;

	return (bot->m_nearestEnemy->v.origin - bot->pev->origin).GetLengthSquared() <= squaredf(distance);
}

static void HandleRadioFollowMe(edict_t* player)
{
	if (!ebot_radio.GetBool() || !IsRadioSourceValid(player))
		return;

	const int maxFollowers = cclamp(ebot_radio_follow_max_bots.GetInt(), 0, 32);
	if (maxFollowers <= 0)
		return;

	const float searchDistance = cclampf(ebot_radio_follow_distance.GetFloat(), 128.0f, 4096.0f);
	const float searchDistanceSq = squaredf(searchDistance);
	const int playerTeam = GetTeam(player);

	CArray<RadioFollowCandidate> candidates;
	for (Bot* const& bot : g_botManager->m_bots)
	{
		if (!bot || !bot->pev || FNullEnt(bot->m_myself))
			continue;

		if (!bot->m_isAlive || bot->m_myself == player)
			continue;

		if (bot->m_team != playerTeam || GetTeam(bot->m_myself) != playerTeam)
			continue;

		if (bot->m_radioFollowUser)
			continue;

		if (ZombieBotHasHumanTargetWithin(bot, 300.0f))
			continue;

		const float distanceSq = (bot->pev->origin - player->v.origin).GetLengthSquared();
		if (distanceSq > searchDistanceSq)
			continue;

		RadioFollowCandidate candidate;
		candidate.bot = bot;
		candidate.distanceSq = distanceSq;
		candidates.Push(candidate);
	}

	int followers = CountRadioFollowers(player);
	if (followers >= maxFollowers)
		return;

	bool answerSent = false;
	while (followers < maxFollowers && !candidates.IsEmpty())
	{
		int16_t nearestIndex = -1;
		float nearestDistanceSq = 999999999.0f;

		for (int16_t i = 0; i < candidates.Size(); i++)
		{
			const RadioFollowCandidate& candidate = candidates.Get(i);
			if (!candidate.bot || candidate.distanceSq >= nearestDistanceSq)
				continue;

			nearestDistanceSq = candidate.distanceSq;
			nearestIndex = i;
		}

		if (nearestIndex < 0)
			break;

		Bot* bot = candidates.Get(nearestIndex).bot;
		candidates.RemoveAt(nearestIndex);
		if (!bot || !bot->StartRadioFollow(player))
			continue;

		followers++;
		if (!answerSent)
		{
			BotPlayRadioMessage(bot, RADIO_AFFIRMATIVE);
			answerSent = true;
		}
	}
}

static void CancelPendingRadioFollow(edict_t* player)
{
	if (FNullEnt(player))
		return;

	const int clientIndex = ENTINDEX(player) - 1;
	if (clientIndex < 0 || clientIndex >= 32)
		return;

	s_pendingFollow[clientIndex].player = nullptr;
	s_pendingFollow[clientIndex].time = 0.0f;
}

static void HandleRadioHoldPosition(edict_t* player)
{
	if (!ebot_radio.GetBool() || !IsRadioSourceValid(player))
		return;

	CancelPendingRadioFollow(player);

	bool answerSent = false;
	for (Bot* const& bot : g_botManager->m_bots)
	{
		if (!bot || !bot->m_isAlive || bot->m_radioFollowUser != player)
			continue;

		if (bot->StartRadioHoldPosition())
		{
			if (!answerSent)
			{
				BotPlayRadioMessage(bot, RADIO_AFFIRMATIVE);
				answerSent = true;
			}
		}
	}
}

static Bot* FindNearestRadioBot(edict_t* player, const float searchDistance)
{
	if (!IsRadioSourceValid(player))
		return nullptr;

	const int playerTeam = GetTeam(player);
	const float searchDistanceSq = squaredf(searchDistance);
	Bot* nearestBot = nullptr;
	float nearestDistanceSq = 999999999.0f;

	for (Bot* const& bot : g_botManager->m_bots)
	{
		if (!bot || !bot->pev || FNullEnt(bot->m_myself))
			continue;

		if (!bot->m_isAlive || bot->m_myself == player)
			continue;

		if (bot->m_team != playerTeam || GetTeam(bot->m_myself) != playerTeam)
			continue;

		const float distanceSq = (bot->pev->origin - player->v.origin).GetLengthSquared();
		if (distanceSq > searchDistanceSq || distanceSq >= nearestDistanceSq)
			continue;

		nearestDistanceSq = distanceSq;
		nearestBot = bot;
	}

	return nearestBot;
}

static void HandleRadioRegroupTeam(edict_t* player)
{
	if (!ebot_radio.GetBool() || !IsRadioSourceValid(player))
		return;

	CancelPendingRadioFollow(player);

	Bot* bot = FindNearestRadioBot(player, 500.0f);
	if (bot)
		bot->StartRadioRegroup();
}

static void ScheduleRadioFollowMe(edict_t* player)
{
	if (!ebot_radio.GetBool() || !IsRadioSourceValid(player))
		return;

	const int clientIndex = ENTINDEX(player) - 1;
	const int maxClients = cmin(engine->GetMaxClients(), 32);
	if (clientIndex < 0 || clientIndex >= maxClients)
		return;

	const float delay = cclampf(ebot_radio_follow_delay.GetFloat(), 0.0f, 10.0f);
	s_pendingFollow[clientIndex].player = player;
	s_pendingFollow[clientIndex].time = engine->GetTime() + delay;
}

void RadioClientCommand(edict_t* ent, const char* command, const char* arg1)
{
	if (!ebot_radio.GetBool() || !IsRadioSourceValid(ent) || IsNullString(command))
		return;

	const int clientIndex = ENTINDEX(ent) - 1;
	const int maxClients = cmin(engine->GetMaxClients(), 32);
	if (clientIndex < 0 || clientIndex >= maxClients)
		return;

	if (!cstrncmp(command, "radio", 5))
	{
		const int menu = catoi(command + 5);
		s_radioMenu[clientIndex] = (menu >= 1 && menu <= 3) ? menu : 0;
		return;
	}

	if (s_radioMenu[clientIndex] == 0 || cstricmp(command, "menuselect"))
		return;

	const int selection = IsNullString(arg1) ? 0 : catoi(arg1);
	const int menu = s_radioMenu[clientIndex];
	s_radioMenu[clientIndex] = 0;

	if (selection <= 0)
		return;

	const int radioCommand = selection + 10 * (menu - 1);
	if (radioCommand == RADIO_FOLLOWME)
		ScheduleRadioFollowMe(ent);
	else if (radioCommand == RADIO_HOLDPOSITION)
		HandleRadioHoldPosition(ent);
	else if (radioCommand == RADIO_REGROUPTEAM)
		HandleRadioRegroupTeam(ent);
}

void RadioClientDisconnected(edict_t* ent)
{
	if (FNullEnt(ent))
		return;

	RadioClientDied(ENTINDEX(ent));
}

void RadioClientDied(const int entityIndex)
{
	const int clientIndex = entityIndex - 1;
	if (clientIndex >= 0 && clientIndex < 32)
	{
		s_radioMenu[clientIndex] = 0;
		s_pendingFollow[clientIndex].player = nullptr;
		s_pendingFollow[clientIndex].time = 0.0f;
	}

	edict_t* ent = nullptr;
	if (entityIndex > 0 && entityIndex <= engine->GetMaxClients())
		ent = INDEXENT(entityIndex);

	for (Bot* const& bot : g_botManager->m_bots)
	{
		if (!bot)
			continue;

		if (bot->m_radioFollowUser == ent)
			bot->ClearRadioFollow();

		if (!FNullEnt(ent) && bot->m_myself == ent)
		{
			bot->ClearRadioFollow();
			bot->ClearRadioHoldPosition();
		}
	}
}

void RadioUpdate(void)
{
	if (!ebot_radio.GetBool())
	{
		if (s_radioWasEnabled)
			RadioResetAll();

		s_radioWasEnabled = false;
		return;
	}

	s_radioWasEnabled = true;

	const float time = engine->GetTime();
	const int maxClients = cmin(engine->GetMaxClients(), 32);

	for (int i = 0; i < maxClients; i++)
	{
		PendingRadioFollow& follow = s_pendingFollow[i];
		if (FNullEnt(follow.player) || follow.time <= 0.0f)
			continue;

		if (follow.time > time)
			continue;

		edict_t* player = follow.player;
		follow.player = nullptr;
		follow.time = 0.0f;

		HandleRadioFollowMe(player);
	}
}

void RadioResetAll(void)
{
	for (int i = 0; i < 32; i++)
	{
		s_radioMenu[i] = 0;
		s_pendingFollow[i].player = nullptr;
		s_pendingFollow[i].time = 0.0f;
	}

	for (Bot* const& bot : g_botManager->m_bots)
	{
		if (bot)
		{
			bot->ClearRadioFollow();
			bot->ClearRadioHoldPosition();
		}
	}
}

bool Bot::StartRadioFollow(edict_t* player)
{
	if (FNullEnt(player) || !IsAlive(player) || !m_isAlive || FNullEnt(m_myself))
		return false;

	const int playerTeam = GetTeam(player);
	if (playerTeam != m_team || (playerTeam != Team::Terrorist && playerTeam != Team::Counter))
		return false;

	m_radioFollowUser = player;
	m_radioFollowRepathTime = 0.0f;
	m_radioFollowGoalIndex = -1;
	m_currentGoalIndex = -1;
	m_radioHoldPosition = false;
	m_zhCampPointIndex = -1;
	m_myMeshWaypoint = -1;
	m_waitForLeaveWaypoint = false;
	m_navNode.Clear();
	ResetStuck();
	return true;
}

void Bot::ClearRadioFollow(void)
{
	m_radioFollowUser = nullptr;
	m_radioFollowRepathTime = 0.0f;
	m_radioFollowGoalIndex = -1;
}

bool Bot::StartRadioHoldPosition(void)
{
	if (!m_isAlive || FNullEnt(m_myself))
		return false;

	ClearRadioFollow();
	m_radioHoldPosition = true;
	m_navNode.Clear();
	m_currentGoalIndex = -1;
	m_zhCampPointIndex = -1;
	m_myMeshWaypoint = -1;
	m_waitForLeaveWaypoint = false;
	m_moveSpeed = 0.0f;
	m_strafeSpeed = 0.0f;
	ResetStuck();
	return true;
}

void Bot::ClearRadioHoldPosition(void)
{
	m_radioHoldPosition = false;
}

bool Bot::UpdateRadioHoldPosition(void)
{
	if (!m_radioHoldPosition)
		return false;

	if (m_isSlowThink && m_isZombieBot && BotHasVisibleHumanEnemyWithin(this, 300.0f))
	{
		BotPlayRadioMessage(this, RADIO_ENEMYSPOTTED);
		ClearRadioHoldPosition();
		return false;
	}

	m_navNode.Clear();
	m_currentGoalIndex = -1;
	m_waitForLeaveWaypoint = false;
	m_moveSpeed = 0.0f;
	m_strafeSpeed = 0.0f;
	ResetStuck();
	return true;
}

bool Bot::StartRadioRegroup(void)
{
	if (!m_isAlive || FNullEnt(m_myself))
		return false;

	const int16_t previousCamp = IsValidWaypoint(m_zhCampPointIndex) ?
		m_zhCampPointIndex : m_myMeshWaypoint;
	const int16_t previousGoal = m_currentGoalIndex;

	ClearRadioFollow();
	ClearRadioHoldPosition();
	m_waitForLeaveWaypoint = false;
	m_zhCampPointIndex = -1;
	m_myMeshWaypoint = -1;
	m_currentGoalIndex = -1;
	m_navNode.Clear();
	ResetStuck();

	int16_t source = m_currentWaypointIndex;
	if (!IsValidWaypoint(source))
		source = FindWaypoint();

	if (!IsValidWaypoint(source))
		return false;

	if (m_isZombieBot)
	{
		int16_t goal = FindGoalZombie();
		FindPath(source, goal);
		if (!m_navNode.IsEmpty())
		{
			FollowPath();
			return true;
		}

		return false;
	}

	if (!g_waypoint || g_waypoint->m_zmHmPoints.IsEmpty())
		return false;

	CArray<int16_t> candidates;
	for (int16_t i = 0; i < g_waypoint->m_zmHmPoints.Size(); i++)
	{
		const int16_t campIndex = g_waypoint->m_zmHmPoints.Get(i);
		if (!IsValidWaypoint(campIndex) || campIndex == source ||
			campIndex == previousCamp || campIndex == previousGoal ||
			candidates.Has(campIndex))
			continue;

		candidates.Push(campIndex);
	}

	while (!candidates.IsEmpty())
	{
		int16_t nearestSlot = -1;
		int16_t nearestCampIndex = -1;
		float nearestDistanceSq = 999999999.0f;

		for (int16_t i = 0; i < candidates.Size(); i++)
		{
			const int16_t campIndex = candidates.Get(i);
			if (!IsValidWaypoint(campIndex))
				continue;

			const float distanceSq =
				(g_waypoint->m_paths[campIndex].origin - g_waypoint->m_paths[source].origin).GetLengthSquared();
			if (distanceSq >= nearestDistanceSq)
				continue;

			nearestDistanceSq = distanceSq;
			nearestCampIndex = campIndex;
			nearestSlot = i;
		}

		if (!IsValidWaypoint(nearestCampIndex))
			break;

		candidates.RemoveAt(nearestSlot);

		int16_t srcIndex = source;
		int16_t destIndex = nearestCampIndex;
		m_navNode.Clear();
		FindPath(srcIndex, destIndex);

		if (m_navNode.IsEmpty() || m_navNode.Last() != nearestCampIndex)
		{
			m_navNode.Clear();
			if (IsValidWaypoint(source) && m_currentWaypointIndex != source)
				ChangeWptIndex(source);
			continue;
		}

		m_zhCampPointIndex = nearestCampIndex;
		m_currentGoalIndex = nearestCampIndex;
		FollowPath();
		return true;
	}

	m_navNode.Clear();
	return false;
}

bool Bot::UpdateRadioFollow(void)
{
	if (FNullEnt(m_radioFollowUser))
	{
		ClearRadioFollow();
		return false;
	}

	if (!m_isAlive || FNullEnt(m_myself) ||
		!IsAlive(m_radioFollowUser) || GetTeam(m_radioFollowUser) != m_team)
	{
		ClearRadioFollow();
		return false;
	}

	if (m_isZombieBot)
	{
		if (BotHasVisibleHumanEnemyWithin(this, 300.0f))
		{
			BotPlayRadioMessage(this, RADIO_ENEMYSPOTTED);
			ClearRadioFollow();
			return false;
		}
	}
	else if (m_hasEnemiesNear && !FNullEnt(m_nearestEnemy) &&
		IsAlive(m_nearestEnemy) && GetTeam(m_nearestEnemy) != m_team)
	{
		if (m_isEnemyReachable || m_enemyDistance < 450.0f)
			return false;
	}

	const Vector leaderOrigin = m_radioFollowUser->v.origin;
	const float distanceSq = (pev->origin - leaderOrigin).GetLengthSquared();

	if (distanceSq <= squaredf(96.0f))
	{
		m_navNode.Clear();
		m_currentGoalIndex = -1;
		m_moveSpeed = 0.0f;
		m_strafeSpeed = 0.0f;
		ResetStuck();
		return true;
	}

	if (distanceSq <= squaredf(300.0f) && IsVisible(leaderOrigin, m_myself))
	{
		m_navNode.Clear();
		m_currentGoalIndex = -1;
		MoveTo(leaderOrigin);
		return true;
	}

	const int16_t goal = GetRadioFollowWaypoint(m_radioFollowUser);
	const float time = engine->GetTime();
	const bool shouldRepath = time >= m_radioFollowRepathTime ||
		m_navNode.IsEmpty() || goal != m_radioFollowGoalIndex;

	if (shouldRepath && IsValidWaypoint(goal))
	{
		int16_t source = m_currentWaypointIndex;
		int16_t destination = goal;
		m_navNode.Clear();
		m_currentGoalIndex = destination;
		m_radioFollowGoalIndex = destination;
		FindPath(source, destination);
		m_radioFollowRepathTime = time + 0.75f;

		if (!m_navNode.IsEmpty())
			FollowPath();
	}

	if (!m_navNode.IsEmpty())
	{
		FollowPath();
		return true;
	}

	if (IsVisible(leaderOrigin, m_myself))
	{
		MoveTo(leaderOrigin);
		return true;
	}

	return false;
}
