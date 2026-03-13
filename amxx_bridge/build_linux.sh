#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# New AMXX SDK layout:
#   public_from_amxx/
#     - IGameConfigs.h, etc.
#     - sdk/amxxmodule.h, sdk/amxxmodule.cpp
AMXX_PUBLIC_DIR="${AMXX_PUBLIC_DIR:-$SCRIPT_DIR/public_from_amxx}"
AMXX_SDK_DIR="${AMXX_SDK_DIR:-$AMXX_PUBLIC_DIR/sdk}"

if [[ ! -f "$AMXX_SDK_DIR/amxxmodule.h" ]]; then
	echo "amxxmodule.h not found in AMXX_SDK_DIR='$AMXX_SDK_DIR'"
	exit 1
fi

if [[ ! -f "$AMXX_SDK_DIR/amxxmodule.cpp" ]]; then
	echo "amxxmodule.cpp not found in AMXX_SDK_DIR='$AMXX_SDK_DIR'"
	exit 1
fi

if [[ ! -f "$AMXX_PUBLIC_DIR/IGameConfigs.h" ]]; then
	echo "IGameConfigs.h not found in AMXX_PUBLIC_DIR='$AMXX_PUBLIC_DIR'"
	exit 1
fi

mkdir -p "$SCRIPT_DIR/build"

g++ -m32 -std=c++11 -O2 -fPIC -shared \
	-static-libstdc++ -static-libgcc \
	-I"$SCRIPT_DIR/src" \
	-I"$AMXX_PUBLIC_DIR" \
	-I"$AMXX_SDK_DIR" \
	"$AMXX_SDK_DIR/amxxmodule.cpp" \
	"$SCRIPT_DIR/src/ebot_bridge.cpp" \
	-o "$SCRIPT_DIR/build/ebot_bridge_amxx_i386.so" \
	-ldl

echo "Built: $SCRIPT_DIR/build/ebot_bridge_amxx_i386.so"
