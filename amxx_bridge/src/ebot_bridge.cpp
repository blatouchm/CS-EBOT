#include "amxxmodule.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

struct Vector {
	float x;
	float y;
	float z;
};

namespace {

#ifdef _WIN32
static HMODULE g_ebotHandle = nullptr;
#else
static void* g_ebotHandle = nullptr;
#endif

static bool g_triedOpen = false;

static void* TryResolveRaw(const char* name) {
	if (!name)
		return nullptr;

#ifdef _WIN32
	void* sym = nullptr;
	if (g_ebotHandle)
		sym = reinterpret_cast<void*>(GetProcAddress(g_ebotHandle, name));

	if (!sym)
		sym = reinterpret_cast<void*>(GetProcAddress(GetModuleHandleA(nullptr), name));

	return sym;
#else
	void* sym = dlsym(RTLD_DEFAULT, name);
	if (sym)
		return sym;

	if (g_ebotHandle)
		sym = dlsym(g_ebotHandle, name);

	return sym;
#endif
}

static void EnsureEbotLoaded() {
	if (g_triedOpen)
		return;

	g_triedOpen = true;

#ifdef _WIN32
	g_ebotHandle = GetModuleHandleA("ebot.dll");
	if (!g_ebotHandle)
		g_ebotHandle = LoadLibraryA("addons\\ebot\\dlls\\ebot.dll");
#else
	g_ebotHandle = dlopen("addons/ebot/dlls/ebot.so", RTLD_NOW | RTLD_GLOBAL);
	if (!g_ebotHandle)
		g_ebotHandle = dlopen("ebot.so", RTLD_NOW | RTLD_GLOBAL);
#endif
}

template <typename T>
static T Resolve(AMX* amx, const char* symbol) {
	EnsureEbotLoaded();
	void* raw = TryResolveRaw(symbol);
	if (!raw) {
		MF_LogError(amx, AMX_ERR_NATIVE, "EBot bridge: missing symbol '%s'", symbol);
		return nullptr;
	}

	return reinterpret_cast<T>(raw);
}

static Vector ReadVector(AMX* amx, cell param) {
	cell* addr = MF_GetAmxAddr(amx, param);
	Vector out;
	out.x = amx_ctof(addr[0]);
	out.y = amx_ctof(addr[1]);
	out.z = amx_ctof(addr[2]);
	return out;
}

static void WriteVector(AMX* amx, cell param, const Vector& v) {
	cell* addr = MF_GetAmxAddr(amx, param);
	addr[0] = amx_ftoc(v.x);
	addr[1] = amx_ftoc(v.y);
	addr[2] = amx_ftoc(v.z);
}

#define RESOLVE_OR_RETURN(fnType, fnVar, symbol, ret) \
	if (!(fnVar)) \
		(fnVar) = Resolve<fnType>(amx, symbol); \
	if (!(fnVar)) \
		return (ret);

#define NATIVE_INT_0(name) \
static cell AMX_NATIVE_CALL n_##name(AMX* amx, cell* params) { \
	using Fn = int (*)(); \
	static Fn fn = nullptr; \
	RESOLVE_OR_RETURN(Fn, fn, #name, 0); \
	return static_cast<cell>(fn()); \
}

#define NATIVE_INT_1I(name) \
static cell AMX_NATIVE_CALL n_##name(AMX* amx, cell* params) { \
	using Fn = int (*)(int); \
	static Fn fn = nullptr; \
	RESOLVE_OR_RETURN(Fn, fn, #name, 0); \
	return static_cast<cell>(fn(static_cast<int>(params[1]))); \
}

#define NATIVE_INT_2I(name) \
static cell AMX_NATIVE_CALL n_##name(AMX* amx, cell* params) { \
	using Fn = int (*)(int, int); \
	static Fn fn = nullptr; \
	RESOLVE_OR_RETURN(Fn, fn, #name, 0); \
	return static_cast<cell>(fn(static_cast<int>(params[1]), static_cast<int>(params[2]))); \
}

#define NATIVE_FLOAT_0(name) \
static cell AMX_NATIVE_CALL n_##name(AMX* amx, cell* params) { \
	using Fn = float (*)(); \
	static Fn fn = nullptr; \
	RESOLVE_OR_RETURN(Fn, fn, #name, 0); \
	return amx_ftoc(fn()); \
}

#define NATIVE_FLOAT_1I(name) \
static cell AMX_NATIVE_CALL n_##name(AMX* amx, cell* params) { \
	using Fn = float (*)(int); \
	static Fn fn = nullptr; \
	RESOLVE_OR_RETURN(Fn, fn, #name, 0); \
	return amx_ftoc(fn(static_cast<int>(params[1]))); \
}

#define NATIVE_FLOAT_2I(name) \
static cell AMX_NATIVE_CALL n_##name(AMX* amx, cell* params) { \
	using Fn = float (*)(int, int); \
	static Fn fn = nullptr; \
	RESOLVE_OR_RETURN(Fn, fn, #name, 0); \
	return amx_ftoc(fn(static_cast<int>(params[1]), static_cast<int>(params[2]))); \
}

#define NATIVE_VOID_1I(name) \
static cell AMX_NATIVE_CALL n_##name(AMX* amx, cell* params) { \
	using Fn = void (*)(int); \
	static Fn fn = nullptr; \
	RESOLVE_OR_RETURN(Fn, fn, #name, 0); \
	fn(static_cast<int>(params[1])); \
	return 1; \
}

#define NATIVE_VOID_2I(name) \
static cell AMX_NATIVE_CALL n_##name(AMX* amx, cell* params) { \
	using Fn = void (*)(int, int); \
	static Fn fn = nullptr; \
	RESOLVE_OR_RETURN(Fn, fn, #name, 0); \
	fn(static_cast<int>(params[1]), static_cast<int>(params[2])); \
	return 1; \
}

NATIVE_INT_0(Amxx_RunEBot)
NATIVE_FLOAT_0(Amxx_EBotVersion)
NATIVE_INT_1I(Amxx_IsEBot)
NATIVE_INT_1I(Amxx_EBotSeesEnemy)
NATIVE_INT_1I(Amxx_EBotSeesEntity)
NATIVE_INT_1I(Amxx_EBotSeesFriend)
NATIVE_INT_1I(Amxx_EBotGetEnemy)
NATIVE_INT_1I(Amxx_EBotGetEntity)
NATIVE_INT_1I(Amxx_EBotGetFriend)
NATIVE_VOID_2I(Amxx_EBotSetEnemy)
NATIVE_VOID_2I(Amxx_EBotSetEntity)
NATIVE_VOID_2I(Amxx_EBotSetFriend)
NATIVE_FLOAT_1I(Amxx_EBotGetEnemyDistance)
NATIVE_FLOAT_1I(Amxx_EBotGetEntityDistance)
NATIVE_FLOAT_1I(Amxx_EBotGetFriendDistance)
NATIVE_INT_1I(Amxx_EBotGetCurrentProcess)
NATIVE_INT_1I(Amxx_EBotGetRememberedProcess)
NATIVE_FLOAT_1I(Amxx_EBotGetCurrentProcessTime)
NATIVE_FLOAT_1I(Amxx_EBotGetRememberedProcessTime)
NATIVE_INT_1I(Amxx_EBotGetOverrideProcessID)
NATIVE_INT_1I(Amxx_EBotHasOverrideChecks)
NATIVE_INT_1I(Amxx_EBotHasOverrideLookAI)
NATIVE_VOID_1I(Amxx_EBotLookAtAround)
NATIVE_VOID_1I(Amxx_EBotCallNewRound)
NATIVE_VOID_2I(Amxx_EBotSetIgnoreClient)
NATIVE_INT_1I(Amxx_EBotIsClientIgnored)
NATIVE_INT_1I(Amxx_EBotGetClientWaypoint)
NATIVE_INT_1I(Amxx_EBotIsClientOwner)
NATIVE_INT_1I(Amxx_EBotIsClientInGame)
NATIVE_INT_1I(Amxx_EBotIsClientAlive)
NATIVE_INT_1I(Amxx_EBotIsEnemyReachable)
NATIVE_VOID_2I(Amxx_EBotSetEnemyReachable)
NATIVE_INT_1I(Amxx_EBotIsStuck)
NATIVE_FLOAT_1I(Amxx_EBotGetStuckTime)
NATIVE_INT_2I(Amxx_EBotGetAmmo)
NATIVE_INT_1I(Amxx_EBotGetAmmoInClip)
NATIVE_INT_1I(Amxx_EBotGetCurrentWeapon)
NATIVE_VOID_1I(Amxx_EBotSelectKnife)
NATIVE_VOID_1I(Amxx_EBotBestWeapon)
NATIVE_INT_1I(Amxx_EBotIsSlowThink)
NATIVE_VOID_2I(Amxx_EBotSetSlowThink)
NATIVE_INT_1I(Amxx_EBotIsZombie)
NATIVE_VOID_2I(Amxx_EBotSetZombie)
NATIVE_INT_1I(Amxx_EBotIsAlive)
NATIVE_VOID_2I(Amxx_EBotSetAlive)
NATIVE_INT_0(Amxx_EBotGetWaypointNumer)
NATIVE_VOID_2I(Amxx_EBotFindPathTo)
NATIVE_VOID_2I(Amxx_EBotFindShortestPathTo)
NATIVE_INT_1I(Amxx_EBotGetCurrentWaypoint)
NATIVE_INT_1I(Amxx_EBotGetGoalWaypoint)
NATIVE_VOID_2I(Amxx_EBotSetCurrentWaypoint)
NATIVE_VOID_2I(Amxx_EBotSetGoalWaypoint)
NATIVE_INT_1I(Amxx_EBotGetCampWaypoint)
NATIVE_INT_1I(Amxx_EBotGetMeshWaypoint)
NATIVE_INT_1I(Amxx_EBotCanFollowPath)
NATIVE_VOID_1I(Amxx_EBotFollowPath)
NATIVE_VOID_1I(Amxx_EBotStopFollowingPath)
NATIVE_VOID_1I(Amxx_EBotShiftPath)
NATIVE_VOID_1I(Amxx_EBotClearPath)
NATIVE_INT_2I(Amxx_EBotGetPath)
NATIVE_INT_1I(Amxx_EBotGetPathLength)
NATIVE_INT_0(Amxx_EBotFindRandomWaypointNoFlags)
NATIVE_FLOAT_2I(Amxx_EBotGetWaypointDistance)
NATIVE_INT_1I(Amxx_EBotIsValidWaypoint)
NATIVE_INT_0(Amxx_EBotIsMatrixReady)
NATIVE_INT_1I(Amxx_EBotIsCamping)
NATIVE_INT_1I(Amxx_EBotRemoveEnemyEntity)

static cell AMX_NATIVE_CALL n_Amxx_EBotRegisterEnemyEntity(AMX* amx, cell* params) {
	using Fn = int (*)(int, int);
	static Fn fn = nullptr;
	RESOLVE_OR_RETURN(Fn, fn, "Amxx_EBotRegisterEnemyEntity", 0);

	const int index = static_cast<int>(params[1]);
	const int targetMask = params[0] >= static_cast<cell>(2 * sizeof(cell))
		? static_cast<int>(params[2])
		: 3;

	return static_cast<cell>(fn(index, targetMask));
}

static cell AMX_NATIVE_CALL n_Amxx_EBotSetLookAt(AMX* amx, cell* params) {
	using Fn = void (*)(int, Vector, Vector);
	static Fn fn = nullptr;
	RESOLVE_OR_RETURN(Fn, fn, "Amxx_EBotSetLookAt", 0);

	const int index = static_cast<int>(params[1]);
	const Vector look = ReadVector(amx, params[2]);
	const Vector vel = ReadVector(amx, params[3]);
	fn(index, look, vel);
	return 1;
}

static cell AMX_NATIVE_CALL n_Amxx_EBotSetCurrentProcess(AMX* amx, cell* params) {
	using Fn = int (*)(int, int, int, float);
	static Fn fn = nullptr;
	RESOLVE_OR_RETURN(Fn, fn, "Amxx_EBotSetCurrentProcess", 0);
	return static_cast<cell>(fn(static_cast<int>(params[1]),
		static_cast<int>(params[2]),
		static_cast<int>(params[3]),
		amx_ctof(params[4])));
}

static cell AMX_NATIVE_CALL n_Amxx_EBotForceCurrentProcess(AMX* amx, cell* params) {
	using Fn = void (*)(int, int);
	static Fn fn = nullptr;
	RESOLVE_OR_RETURN(Fn, fn, "Amxx_EBotForceCurrentProcess", 0);
	fn(static_cast<int>(params[1]), static_cast<int>(params[2]));
	return 1;
}

static cell AMX_NATIVE_CALL n_Amxx_EBotFinishCurrentProcess(AMX* amx, cell* params) {
	using Fn = void (*)(int);
	static Fn fn = nullptr;
	RESOLVE_OR_RETURN(Fn, fn, "Amxx_EBotFinishCurrentProcess", 0);
	fn(static_cast<int>(params[1]));
	return 1;
}

static cell AMX_NATIVE_CALL n_Amxx_EBotOverrideCurrentProcess(AMX* amx, cell* params) {
	using Fn = int (*)(int, int, float, int, int, int);
	static Fn fn = nullptr;
	RESOLVE_OR_RETURN(Fn, fn, "Amxx_EBotOverrideCurrentProcess", 0);
	return static_cast<cell>(fn(static_cast<int>(params[1]),
		static_cast<int>(params[2]),
		amx_ctof(params[3]),
		static_cast<int>(params[4]),
		static_cast<int>(params[5]),
		static_cast<int>(params[6])));
}

static cell AMX_NATIVE_CALL n_Amxx_EBotForceFireWeapon(AMX* amx, cell* params) {
	using Fn = void (*)(int, float);
	static Fn fn = nullptr;
	RESOLVE_OR_RETURN(Fn, fn, "Amxx_EBotForceFireWeapon", 0);
	fn(static_cast<int>(params[1]), amx_ctof(params[2]));
	return 1;
}

static cell AMX_NATIVE_CALL n_Amxx_EBotGetWaypointOrigin(AMX* amx, cell* params) {
	using Fn = void (*)(int, Vector**);
	static Fn fn = nullptr;
	RESOLVE_OR_RETURN(Fn, fn, "Amxx_EBotGetWaypointOrigin", 0);

	Vector* out = nullptr;
	fn(static_cast<int>(params[1]), &out);
	if (!out)
		return 0;

	WriteVector(amx, params[2], *out);
	return 1;
}

static cell AMX_NATIVE_CALL n_Amxx_EBotGetWaypointFlags(AMX* amx, cell* params) {
	using Fn = void (*)(int, int**);
	static Fn fn = nullptr;
	RESOLVE_OR_RETURN(Fn, fn, "Amxx_EBotGetWaypointFlags", 0);

	int* out = nullptr;
	fn(static_cast<int>(params[1]), &out);
	if (!out)
		return 0;

	*MF_GetAmxAddr(amx, params[2]) = static_cast<cell>(*out);
	return 1;
}

static cell AMX_NATIVE_CALL n_Amxx_EBotGetWaypointRadius(AMX* amx, cell* params) {
	using Fn = void (*)(int, int**);
	static Fn fn = nullptr;
	RESOLVE_OR_RETURN(Fn, fn, "Amxx_EBotGetWaypointRadius", 0);

	int* out = nullptr;
	fn(static_cast<int>(params[1]), &out);
	if (!out)
		return 0;

	*MF_GetAmxAddr(amx, params[2]) = static_cast<cell>(*out);
	return 1;
}

static cell AMX_NATIVE_CALL n_Amxx_EBotGetWaypointMesh(AMX* amx, cell* params) {
	using Fn = void (*)(int, int**);
	static Fn fn = nullptr;
	RESOLVE_OR_RETURN(Fn, fn, "Amxx_EBotGetWaypointMesh", 0);

	int* out = nullptr;
	fn(static_cast<int>(params[1]), &out);
	if (!out)
		return 0;

	*MF_GetAmxAddr(amx, params[2]) = static_cast<cell>(*out);
	return 1;
}

static cell AMX_NATIVE_CALL n_Amxx_EBotGetWaypointConnections(AMX* amx, cell* params) {
	using Fn = void (*)(int, int**, int);
	static Fn fn = nullptr;
	RESOLVE_OR_RETURN(Fn, fn, "Amxx_EBotGetWaypointConnections", 0);

	int* out = nullptr;
	fn(static_cast<int>(params[1]), &out, static_cast<int>(params[2]));
	if (!out)
		return 0;

	*MF_GetAmxAddr(amx, params[3]) = static_cast<cell>(*out);
	return 1;
}

static cell AMX_NATIVE_CALL n_Amxx_EBotGetWaypointConnectionFlags(AMX* amx, cell* params) {
	using Fn = void (*)(int, int**, int);
	static Fn fn = nullptr;
	RESOLVE_OR_RETURN(Fn, fn, "Amxx_EBotGetWaypointConnectionFlags", 0);

	int* out = nullptr;
	fn(static_cast<int>(params[1]), &out, static_cast<int>(params[2]));
	if (!out)
		return 0;

	*MF_GetAmxAddr(amx, params[3]) = static_cast<cell>(*out);
	return 1;
}

static cell AMX_NATIVE_CALL n_Amxx_EBotGetWaypointGravity(AMX* amx, cell* params) {
	using Fn = void (*)(int, float**);
	static Fn fn = nullptr;
	RESOLVE_OR_RETURN(Fn, fn, "Amxx_EBotGetWaypointGravity", 0);

	float* out = nullptr;
	fn(static_cast<int>(params[1]), &out);
	if (!out)
		return 0;

	*MF_GetAmxAddr(amx, params[2]) = amx_ftoc(*out);
	return 1;
}

static cell AMX_NATIVE_CALL n_Amxx_EBotMoveTo(AMX* amx, cell* params) {
	using Fn = void (*)(int, Vector, int);
	static Fn fn = nullptr;
	RESOLVE_OR_RETURN(Fn, fn, "Amxx_EBotMoveTo", 0);

	fn(static_cast<int>(params[1]), ReadVector(amx, params[2]), static_cast<int>(params[3]));
	return 1;
}

static cell AMX_NATIVE_CALL n_Amxx_EBotMoveOut(AMX* amx, cell* params) {
	using Fn = void (*)(int, Vector, int);
	static Fn fn = nullptr;
	RESOLVE_OR_RETURN(Fn, fn, "Amxx_EBotMoveOut", 0);

	fn(static_cast<int>(params[1]), ReadVector(amx, params[2]), static_cast<int>(params[3]));
	return 1;
}

static cell AMX_NATIVE_CALL n_Amxx_EBotIsNodeReachable(AMX* amx, cell* params) {
	using Fn = int (*)(Vector, Vector);
	static Fn fn = nullptr;
	RESOLVE_OR_RETURN(Fn, fn, "Amxx_EBotIsNodeReachable", 0);

	return static_cast<cell>(fn(ReadVector(amx, params[1]), ReadVector(amx, params[2])));
}

static cell AMX_NATIVE_CALL n_Amxx_EBotFindNearestWaypoint(AMX* amx, cell* params) {
	using Fn = int (*)(Vector, float);
	static Fn fn = nullptr;
	RESOLVE_OR_RETURN(Fn, fn, "Amxx_EBotFindNearestWaypoint", 0);

	return static_cast<cell>(fn(ReadVector(amx, params[1]), amx_ctof(params[2])));
}

static cell AMX_NATIVE_CALL n_Amxx_EBotFindFarestWaypoint(AMX* amx, cell* params) {
	using Fn = int (*)(Vector, float);
	static Fn fn = nullptr;
	RESOLVE_OR_RETURN(Fn, fn, "Amxx_EBotFindFarestWaypoint", 0);

	return static_cast<cell>(fn(ReadVector(amx, params[1]), amx_ctof(params[2])));
}

static cell AMX_NATIVE_CALL n_Amxx_EBotFindNearestWaypointToEntity(AMX* amx, cell* params) {
	using Fn = int (*)(Vector, float, int);
	static Fn fn = nullptr;
	RESOLVE_OR_RETURN(Fn, fn, "Amxx_EBotFindNearestWaypointToEntity", 0);

	return static_cast<cell>(fn(ReadVector(amx, params[1]), amx_ctof(params[2]), static_cast<int>(params[3])));
}

static cell AMX_NATIVE_CALL n_Amxx_EBotClearRegisteredEnemyEntities(AMX* amx, cell* params) {
	using Fn = void (*)();
	static Fn fn = nullptr;
	RESOLVE_OR_RETURN(Fn, fn, "Amxx_EBotClearRegisteredEnemyEntities", 0);
	fn();
	return 1;
}

AMX_NATIVE_INFO g_ebotNatives[] = {
	{"Amxx_RunEBot", n_Amxx_RunEBot},
	{"Amxx_EBotVersion", n_Amxx_EBotVersion},
	{"Amxx_IsEBot", n_Amxx_IsEBot},
	{"Amxx_EBotSeesEnemy", n_Amxx_EBotSeesEnemy},
	{"Amxx_EBotSeesEntity", n_Amxx_EBotSeesEntity},
	{"Amxx_EBotSeesFriend", n_Amxx_EBotSeesFriend},
	{"Amxx_EBotGetEnemy", n_Amxx_EBotGetEnemy},
	{"Amxx_EBotGetEntity", n_Amxx_EBotGetEntity},
	{"Amxx_EBotGetFriend", n_Amxx_EBotGetFriend},
	{"Amxx_EBotSetEnemy", n_Amxx_EBotSetEnemy},
	{"Amxx_EBotSetEntity", n_Amxx_EBotSetEntity},
	{"Amxx_EBotSetFriend", n_Amxx_EBotSetFriend},
	{"Amxx_EBotGetEnemyDistance", n_Amxx_EBotGetEnemyDistance},
	{"Amxx_EBotGetEntityDistance", n_Amxx_EBotGetEntityDistance},
	{"Amxx_EBotGetFriendDistance", n_Amxx_EBotGetFriendDistance},
	{"Amxx_EBotSetLookAt", n_Amxx_EBotSetLookAt},
	{"Amxx_EBotGetCurrentProcess", n_Amxx_EBotGetCurrentProcess},
	{"Amxx_EBotGetRememberedProcess", n_Amxx_EBotGetRememberedProcess},
	{"Amxx_EBotGetCurrentProcessTime", n_Amxx_EBotGetCurrentProcessTime},
	{"Amxx_EBotGetRememberedProcessTime", n_Amxx_EBotGetRememberedProcessTime},
	{"Amxx_EBotSetCurrentProcess", n_Amxx_EBotSetCurrentProcess},
	{"Amxx_EBotForceCurrentProcess", n_Amxx_EBotForceCurrentProcess},
	{"Amxx_EBotFinishCurrentProcess", n_Amxx_EBotFinishCurrentProcess},
	{"Amxx_EBotOverrideCurrentProcess", n_Amxx_EBotOverrideCurrentProcess},
	{"Amxx_EBotGetOverrideProcessID", n_Amxx_EBotGetOverrideProcessID},
	{"Amxx_EBotHasOverrideChecks", n_Amxx_EBotHasOverrideChecks},
	{"Amxx_EBotHasOverrideLookAI", n_Amxx_EBotHasOverrideLookAI},
	{"Amxx_EBotForceFireWeapon", n_Amxx_EBotForceFireWeapon},
	{"Amxx_EBotLookAtAround", n_Amxx_EBotLookAtAround},
	{"Amxx_EBotCallNewRound", n_Amxx_EBotCallNewRound},
	{"Amxx_EBotSetIgnoreClient", n_Amxx_EBotSetIgnoreClient},
	{"Amxx_EBotIsClientIgnored", n_Amxx_EBotIsClientIgnored},
	{"Amxx_EBotGetClientWaypoint", n_Amxx_EBotGetClientWaypoint},
	{"Amxx_EBotIsClientOwner", n_Amxx_EBotIsClientOwner},
	{"Amxx_EBotIsClientInGame", n_Amxx_EBotIsClientInGame},
	{"Amxx_EBotIsClientAlive", n_Amxx_EBotIsClientAlive},
	{"Amxx_EBotIsEnemyReachable", n_Amxx_EBotIsEnemyReachable},
	{"Amxx_EBotSetEnemyReachable", n_Amxx_EBotSetEnemyReachable},
	{"Amxx_EBotIsStuck", n_Amxx_EBotIsStuck},
	{"Amxx_EBotGetStuckTime", n_Amxx_EBotGetStuckTime},
	{"Amxx_EBotGetAmmo", n_Amxx_EBotGetAmmo},
	{"Amxx_EBotGetAmmoInClip", n_Amxx_EBotGetAmmoInClip},
	{"Amxx_EBotGetCurrentWeapon", n_Amxx_EBotGetCurrentWeapon},
	{"Amxx_EBotSelectKnife", n_Amxx_EBotSelectKnife},
	{"Amxx_EBotBestWeapon", n_Amxx_EBotBestWeapon},
	{"Amxx_EBotIsSlowThink", n_Amxx_EBotIsSlowThink},
	{"Amxx_EBotSetSlowThink", n_Amxx_EBotSetSlowThink},
	{"Amxx_EBotIsZombie", n_Amxx_EBotIsZombie},
	{"Amxx_EBotSetZombie", n_Amxx_EBotSetZombie},
	{"Amxx_EBotIsAlive", n_Amxx_EBotIsAlive},
	{"Amxx_EBotSetAlive", n_Amxx_EBotSetAlive},
	{"Amxx_EBotGetWaypointNumer", n_Amxx_EBotGetWaypointNumer},
	{"Amxx_EBotGetWaypointOrigin", n_Amxx_EBotGetWaypointOrigin},
	{"Amxx_EBotGetWaypointFlags", n_Amxx_EBotGetWaypointFlags},
	{"Amxx_EBotGetWaypointRadius", n_Amxx_EBotGetWaypointRadius},
	{"Amxx_EBotGetWaypointMesh", n_Amxx_EBotGetWaypointMesh},
	{"Amxx_EBotGetWaypointConnections", n_Amxx_EBotGetWaypointConnections},
	{"Amxx_EBotGetWaypointConnectionFlags", n_Amxx_EBotGetWaypointConnectionFlags},
	{"Amxx_EBotGetWaypointGravity", n_Amxx_EBotGetWaypointGravity},
	{"Amxx_EBotMoveTo", n_Amxx_EBotMoveTo},
	{"Amxx_EBotMoveOut", n_Amxx_EBotMoveOut},
	{"Amxx_EBotFindPathTo", n_Amxx_EBotFindPathTo},
	{"Amxx_EBotFindShortestPathTo", n_Amxx_EBotFindShortestPathTo},
	{"Amxx_EBotGetCurrentWaypoint", n_Amxx_EBotGetCurrentWaypoint},
	{"Amxx_EBotGetGoalWaypoint", n_Amxx_EBotGetGoalWaypoint},
	{"Amxx_EBotSetCurrentWaypoint", n_Amxx_EBotSetCurrentWaypoint},
	{"Amxx_EBotSetGoalWaypoint", n_Amxx_EBotSetGoalWaypoint},
	{"Amxx_EBotGetCampWaypoint", n_Amxx_EBotGetCampWaypoint},
	{"Amxx_EBotGetMeshWaypoint", n_Amxx_EBotGetMeshWaypoint},
	{"Amxx_EBotCanFollowPath", n_Amxx_EBotCanFollowPath},
	{"Amxx_EBotFollowPath", n_Amxx_EBotFollowPath},
	{"Amxx_EBotStopFollowingPath", n_Amxx_EBotStopFollowingPath},
	{"Amxx_EBotShiftPath", n_Amxx_EBotShiftPath},
	{"Amxx_EBotClearPath", n_Amxx_EBotClearPath},
	{"Amxx_EBotGetPath", n_Amxx_EBotGetPath},
	{"Amxx_EBotGetPathLength", n_Amxx_EBotGetPathLength},
	{"Amxx_EBotIsNodeReachable", n_Amxx_EBotIsNodeReachable},
	{"Amxx_EBotFindNearestWaypoint", n_Amxx_EBotFindNearestWaypoint},
	{"Amxx_EBotFindRandomWaypointNoFlags", n_Amxx_EBotFindRandomWaypointNoFlags},
	{"Amxx_EBotFindFarestWaypoint", n_Amxx_EBotFindFarestWaypoint},
	{"Amxx_EBotFindNearestWaypointToEntity", n_Amxx_EBotFindNearestWaypointToEntity},
	{"Amxx_EBotGetWaypointDistance", n_Amxx_EBotGetWaypointDistance},
	{"Amxx_EBotIsValidWaypoint", n_Amxx_EBotIsValidWaypoint},
	{"Amxx_EBotIsMatrixReady", n_Amxx_EBotIsMatrixReady},
	{"Amxx_EBotIsCamping", n_Amxx_EBotIsCamping},
	{"Amxx_EBotRegisterEnemyEntity", n_Amxx_EBotRegisterEnemyEntity},
	{"Amxx_EBotRemoveEnemyEntity", n_Amxx_EBotRemoveEnemyEntity},
	{"Amxx_EBotClearRegisteredEnemyEntities", n_Amxx_EBotClearRegisteredEnemyEntities},
	{nullptr, nullptr}
};

} // namespace

void OnAmxxAttach() {
	MF_AddNatives(g_ebotNatives);
}

void OnAmxxDetach() {
}
