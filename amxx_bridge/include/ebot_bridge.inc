#if defined _ebot_bridge_included
	#endinput
#endif
#define _ebot_bridge_included

// Returns true when bridge + EBot exports are available.
native bool:Amxx_RunEBot();
// Returns EBot version as float (e.g. 1.15).
native Float:Amxx_EBotVersion();

// Returns 1 if index is EBot, else 0.
native Amxx_IsEBot(index);
// Returns 1 if bot currently sees enemy, else 0.
native Amxx_EBotSeesEnemy(index);
// Returns 1 if bot currently sees tracked entity, else 0.
native Amxx_EBotSeesEntity(index);
// Returns 1 if bot currently sees tracked friend, else 0.
native Amxx_EBotSeesFriend(index);

// Returns current enemy entity index, or 0 if none.
native Amxx_EBotGetEnemy(index);
// Returns current tracked entity index, or 0 if none.
native Amxx_EBotGetEntity(index);
// Returns current tracked friend entity index, or 0 if none.
native Amxx_EBotGetFriend(index);

// Sets enemy entity for bot.
native Amxx_EBotSetEnemy(index, ent);
// Sets tracked entity for bot.
native Amxx_EBotSetEntity(index, ent);
// Sets friend entity for bot.
native Amxx_EBotSetFriend(index, ent);

// Returns distance to enemy (0.0 when invalid/unset).
native Float:Amxx_EBotGetEnemyDistance(index);
// Returns distance to tracked entity (0.0 when invalid/unset).
native Float:Amxx_EBotGetEntityDistance(index);
// Returns distance to tracked friend (0.0 when invalid/unset).
native Float:Amxx_EBotGetFriendDistance(index);

// Forces look direction: look origin + look velocity.
native Amxx_EBotSetLookAt(index, const Float:look[3], const Float:vel[3]);

// Returns current process id.
native Amxx_EBotGetCurrentProcess(index);
// Returns remembered process id.
native Amxx_EBotGetRememberedProcess(index);
// Returns current process end time / timer value.
native Float:Amxx_EBotGetCurrentProcessTime(index);
// Returns remembered process time value.
native Float:Amxx_EBotGetRememberedProcessTime(index);

// Sets current process (optionally remembered) until time.
native Amxx_EBotSetCurrentProcess(index, process, remember, Float:time);
// Forces process immediately.
native Amxx_EBotForceCurrentProcess(index, process);
// Finishes current process immediately.
native Amxx_EBotFinishCurrentProcess(index);
// Overrides process rules; returns override process id/status.
native Amxx_EBotOverrideCurrentProcess(index, remember, Float:time, customID, defaultLookAI, defaultChecks);
// Returns active override process id.
native Amxx_EBotGetOverrideProcessID(index);
// Returns 1 if override checks are enabled, else 0.
native Amxx_EBotHasOverrideChecks(index);
// Returns 1 if override look AI is enabled, else 0.
native Amxx_EBotHasOverrideLookAI(index);

// Forces weapon fire logic for target distance.
native Amxx_EBotForceFireWeapon(index, Float:targetDistance);
// Triggers looking around behavior.
native Amxx_EBotLookAtAround(index);
// Triggers bot new-round reset routine.
native Amxx_EBotCallNewRound(index);

// Marks client as ignored / not ignored.
native Amxx_EBotSetIgnoreClient(index, ignore);
// Returns 1 if client is ignored, else 0.
native Amxx_EBotIsClientIgnored(index);
// Returns nearest/current waypoint for client (or -1).
native Amxx_EBotGetClientWaypoint(index);
// Returns 1 if client entity is bot owner index, else 0.
native Amxx_EBotIsClientOwner(index);
// Returns 1 if client is in game, else 0.
native Amxx_EBotIsClientInGame(index);
// Returns 1 if client is alive, else 0.
native Amxx_EBotIsClientAlive(index);

// Returns 1 if enemy is reachable, else 0.
native Amxx_EBotIsEnemyReachable(index);
// Sets enemy reachable flag for bot.
native Amxx_EBotSetEnemyReachable(index, reachable);

// Returns 1 if bot is marked as stuck, else 0.
native Amxx_EBotIsStuck(index);
// Returns stuck timer in seconds.
native Float:Amxx_EBotGetStuckTime(index);

// Returns ammo reserve for given weapon id.
native Amxx_EBotGetAmmo(index, weapon);
// Returns ammo in current clip.
native Amxx_EBotGetAmmoInClip(index);
// Returns current weapon id.
native Amxx_EBotGetCurrentWeapon(index);
// Selects knife weapon.
native Amxx_EBotSelectKnife(index);
// Selects best available weapon.
native Amxx_EBotBestWeapon(index);
// Returns 1 if slow think mode is enabled, else 0.
native Amxx_EBotIsSlowThink(index);
// Enables/disables slow think mode.
native Amxx_EBotSetSlowThink(index, slowThink);
// Returns 1 if bot is zombie, else 0.
native Amxx_EBotIsZombie(index);
// Sets zombie flag.
native Amxx_EBotSetZombie(index, zombie);
// Returns 1 if bot alive flag is set, else 0.
native Amxx_EBotIsAlive(index);
// Sets alive flag.
native Amxx_EBotSetAlive(index, alive);

// Returns total waypoint count.
native Amxx_EBotGetWaypointNumer();
// Returns true and writes waypoint origin to origin[3].
native bool:Amxx_EBotGetWaypointOrigin(index, Float:origin[3]);
// Returns true and writes waypoint flags.
native bool:Amxx_EBotGetWaypointFlags(index, &flags);
// Returns true and writes waypoint radius.
native bool:Amxx_EBotGetWaypointRadius(index, &radius);
// Returns true and writes waypoint mesh id.
native bool:Amxx_EBotGetWaypointMesh(index, &mesh);
// Returns true and writes connected waypoint index for path slot.
native bool:Amxx_EBotGetWaypointConnections(index, pathSlot, &targetIndex);
// Returns true and writes connection flags for path slot.
native bool:Amxx_EBotGetWaypointConnectionFlags(index, pathSlot, &connectionFlags);
// Returns true and writes waypoint gravity.
native bool:Amxx_EBotGetWaypointGravity(index, &Float:gravity);

// Moves bot toward world origin.
native Amxx_EBotMoveTo(index, const Float:origin[3], checkStuck = 1);
// Moves bot away from world origin.
native Amxx_EBotMoveOut(index, const Float:origin[3], checkStuck = 1);
// Builds path to goal waypoint.
native Amxx_EBotFindPathTo(index, goal);
// Builds shortest path to goal waypoint.
native Amxx_EBotFindShortestPathTo(index, goal);

// Returns bot current waypoint index.
native Amxx_EBotGetCurrentWaypoint(index);
// Returns bot goal waypoint index.
native Amxx_EBotGetGoalWaypoint(index);
// Sets bot current waypoint index.
native Amxx_EBotSetCurrentWaypoint(index, waypoint);
// Sets bot goal waypoint index.
native Amxx_EBotSetGoalWaypoint(index, waypoint);
// Returns bot camp waypoint index.
native Amxx_EBotGetCampWaypoint(index);
// Returns bot mesh waypoint index.
native Amxx_EBotGetMeshWaypoint(index);

// Returns 1 if bot can follow path, else 0.
native Amxx_EBotCanFollowPath(index);
// Starts following prepared path.
native Amxx_EBotFollowPath(index);
// Stops following path.
native Amxx_EBotStopFollowingPath(index);
// Shifts path to next node.
native Amxx_EBotShiftPath(index);
// Clears current path.
native Amxx_EBotClearPath(index);
// Returns waypoint index from bot path by slot.
native Amxx_EBotGetPath(index, path);
// Returns current path length.
native Amxx_EBotGetPathLength(index);

// Returns 1 if src->dest is reachable by node test, else 0.
native Amxx_EBotIsNodeReachable(const Float:src[3], const Float:dest[3]);
// Returns nearest waypoint index to origin with min distance filter.
native Amxx_EBotFindNearestWaypoint(const Float:origin[3], Float:minDistance);
// Returns random waypoint index with no flags (or -1 on fail).
native Amxx_EBotFindRandomWaypointNoFlags();
// Returns farthest waypoint index from origin with min distance filter.
native Amxx_EBotFindFarestWaypoint(const Float:origin[3], Float:minDistance);
// Returns nearest waypoint index to origin, excluding entity context.
native Amxx_EBotFindNearestWaypointToEntity(const Float:origin[3], Float:minDistance, entityIndex);
// Returns path matrix distance between waypoints.
native Float:Amxx_EBotGetWaypointDistance(srcIndex, destIndex);
// Returns 1 if waypoint index is valid, else 0.
native Amxx_EBotIsValidWaypoint(waypoint);
// Returns 1 if waypoint matrix is ready, else 0.
native Amxx_EBotIsMatrixReady();
// Returns 1 if bot is camping, else 0.
native Amxx_EBotIsCamping(index);

// Registers entity as enemy candidate; returns status.
native Amxx_EBotRegisterEnemyEntity(index);
// Removes entity from enemy candidates; returns status.
native Amxx_EBotRemoveEnemyEntity(index);
// Clears all registered enemy entities.
native Amxx_EBotClearRegisteredEnemyEntities();

// Helper: true when bridge and EBot are available.
stock bool:Amxx_EBotBridgeLoaded()
{
	return Amxx_RunEBot();
}
