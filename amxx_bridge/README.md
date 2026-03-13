# EBot AMXX Bridge

Bridge module for AMX Mod X that registers `Amxx_EBot...` natives and forwards them to exports from `ebot.so` / `ebot.dll`.

## Contents

- `src/ebot_bridge.cpp`: AMXX module
- `src/moduleconfig.in`: module configuration
- `src/moduleconfig.h`: wrapper for `moduleconfig.in`
- `include/ebot_bridge.inc`: include file for `.sma` plugins
- `build_linux.sh`: simple Linux build script
- `build_windows.bat`: simple Windows build script (MSVC x86)

## Linux build

1. Prepare the AMXX public SDK tree (default: `amxx_bridge/public_from_amxx`).
2. Run the build (paths are optional):

```bash
cd amxx_bridge
export AMXX_PUBLIC_DIR=/path/to/public_from_amxx
export AMXX_SDK_DIR=/path/to/public_from_amxx/sdk
./build_linux.sh
```

Output:
- `build/ebot_bridge_amxx_i386.so`

## Windows build

1. Open `x86 Native Tools Command Prompt for VS` (32-bit toolchain).
2. Run the build (paths are optional):

```bat
cd amxx_bridge
set AMXX_PUBLIC_DIR=C:\path\to\public_from_amxx
set AMXX_SDK_DIR=C:\path\to\public_from_amxx\sdk
build_windows.bat
```

Output:
- `build_win\ebot_bridge_amxx.dll`

Note: the Windows script uses static runtime linking (`/MT`), so `VCRUNTIME140.dll` and `api-ms-win-crt-*` should not be required at runtime.

## Server installation

1. Upload the compiled module to:
- `cstrike/addons/amxmodx/modules/`

2. Copy `include/ebot_bridge.inc` to:
- `cstrike/addons/amxmodx/scripting/include/`

3. Add this entry to `cstrike/addons/amxmodx/configs/modules.ini`:

```ini
ebot_bridge
```

4. Make sure the EBot metamod plugin is loaded (`meta list`), then restart the server.

## Plugin usage

```pawn
#include <ebot_bridge>

public plugin_init()
{
	if (!Amxx_EBotBridgeLoaded())
	{
		set_fail_state("EBot bridge/EBot is not loaded");
	}
}
```

## Notes

- For vector-based functions, the bridge uses Pawn arrays like `Float:vec[3]`.
- `Amxx_EBotGetWaypointOrigin`/`Flags`/`Radius`/`Mesh`/`...` return `bool` success and write values to out parameters.
- Bot indexes follow the EBot API (typically `1..32`), waypoint indexes are `0-based`.
