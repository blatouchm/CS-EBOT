#if defined _ebot_bridge_included
	#endinput
#endif
#define _ebot_bridge_included

native bool:Amxx_RunEBot();
native Float:Amxx_EBotVersion();

native Amxx_IsEBot(index);
native Amxx_EBotSeesEnemy(index);
native Amxx_EBotSeesEntity(index);
native Amxx_EBotSeesFriend(index);

native Amxx_EBotGetEnemy(index);
native Amxx_EBotGetEntity(index);
native Amxx_EBotGetFriend(index);

native Amxx_EBotSetEnemy(index, ent);
native Amxx_EBotSetEntity(index, ent);
native Amxx_EBotSetFriend(index, ent);

native Float:Amxx_EBotGetEnemyDistance(index);
native Float:Amxx_EBotGetEntityDistance(index);
native Float:Amxx_EBotGetFriendDistance(index);

native Amxx_EBotSetLookAt(index, const Float:look[3], const Float:vel[3]);

native Amxx_EBotGetCurrentProcess(index);
native Amxx_EBotGetRememberedProcess(index);
native Float:Amxx_EBotGetCurrentProcessTime(index);
native Float:Amxx_EBotGetRememberedProcessTime(index);

native Amxx_EBotSetCurrentProcess(index, process, remember, Float:time);
native Amxx_EBotForceCurrentProcess(index, process);
native Amxx_EBotFinishCurrentProcess(index);
native Amxx_EBotOverrideCurrentProcess(index, remember, Float:time, customID, defaultLookAI, defaultChecks);
native Amxx_EBotGetOverrideProcessID(index);
native Amxx_EBotHasOverrideChecks(index);
native Amxx_EBotHasOverrideLookAI(index);

native Amxx_EBotForceFireWeapon(index, Float:targetDistance);
native Amxx_EBotLookAtAround(index);
native Amxx_EBotCallNewRound(index);

native Amxx_EBotSetIgnoreClient(index, ignore);
native Amxx_EBotIsClientIgnored(index);
native Amxx_EBotGetClientWaypoint(index);
native Amxx_EBotIsClientOwner(index);
native Amxx_EBotIsClientInGame(index);
native Amxx_EBotIsClientAlive(index);

native Amxx_EBotIsEnemyReachable(index);
native Amxx_EBotSetEnemyReachable(index, reachable);

native Amxx_EBotIsStuck(index);
native Float:Amxx_EBotGetStuckTime(index);

native Amxx_EBotGetAmmo(index, weapon);
native Amxx_EBotGetAmmoInClip(index);
native Amxx_EBotGetCurrentWeapon(index);
native Amxx_EBotSelectKnife(index);
native Amxx_EBotBestWeapon(index);
native Amxx_EBotIsSlowThink(index);
native Amxx_EBotSetSlowThink(index, slowThink);
native Amxx_EBotIsZombie(index);
native Amxx_EBotSetZombie(index, zombie);
native Amxx_EBotIsAlive(index);
native Amxx_EBotSetAlive(index, alive);

native Amxx_EBotGetWaypointNumer();
native bool:Amxx_EBotGetWaypointOrigin(index, Float:origin[3]);
native bool:Amxx_EBotGetWaypointFlags(index, &flags);
native bool:Amxx_EBotGetWaypointRadius(index, &radius);
native bool:Amxx_EBotGetWaypointMesh(index, &mesh);
native bool:Amxx_EBotGetWaypointConnections(index, pathSlot, &targetIndex);
native bool:Amxx_EBotGetWaypointConnectionFlags(index, pathSlot, &connectionFlags);
native bool:Amxx_EBotGetWaypointGravity(index, &Float:gravity);

native Amxx_EBotMoveTo(index, const Float:origin[3], checkStuck = 1);
native Amxx_EBotMoveOut(index, const Float:origin[3], checkStuck = 1);
native Amxx_EBotFindPathTo(index, goal);
native Amxx_EBotFindShortestPathTo(index, goal);

native Amxx_EBotGetCurrentWaypoint(index);
native Amxx_EBotGetGoalWaypoint(index);
native Amxx_EBotSetCurrentWaypoint(index, waypoint);
native Amxx_EBotSetGoalWaypoint(index, waypoint);
native Amxx_EBotGetCampWaypoint(index);
native Amxx_EBotGetMeshWaypoint(index);

native Amxx_EBotCanFollowPath(index);
native Amxx_EBotFollowPath(index);
native Amxx_EBotStopFollowingPath(index);
native Amxx_EBotShiftPath(index);
native Amxx_EBotClearPath(index);
native Amxx_EBotGetPath(index, path);
native Amxx_EBotGetPathLength(index);

native Amxx_EBotIsNodeReachable(const Float:src[3], const Float:dest[3]);
native Amxx_EBotFindNearestWaypoint(const Float:origin[3], Float:minDistance);
native Amxx_EBotFindRandomWaypointNoFlags();
native Amxx_EBotFindFarestWaypoint(const Float:origin[3], Float:minDistance);
native Amxx_EBotFindNearestWaypointToEntity(const Float:origin[3], Float:minDistance, entityIndex);
native Float:Amxx_EBotGetWaypointDistance(srcIndex, destIndex);
native Amxx_EBotIsValidWaypoint(waypoint);
native Amxx_EBotIsMatrixReady();
native Amxx_EBotIsCamping(index);

native Amxx_EBotRegisterEnemyEntity(index);
native Amxx_EBotRemoveEnemyEntity(index);
native Amxx_EBotClearRegisteredEnemyEntities();

stock bool:Amxx_EBotBridgeLoaded()
{
	return Amxx_RunEBot();
}
