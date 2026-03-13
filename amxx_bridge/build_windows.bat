@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
if "%AMXX_PUBLIC_DIR%"=="" set "AMXX_PUBLIC_DIR=%SCRIPT_DIR%public_from_amxx"
if "%AMXX_SDK_DIR%"=="" set "AMXX_SDK_DIR=%AMXX_PUBLIC_DIR%\sdk"

if not exist "%AMXX_SDK_DIR%\amxxmodule.h" (
	echo amxxmodule.h not found in AMXX_SDK_DIR="%AMXX_SDK_DIR%"
	exit /b 1
)

if not exist "%AMXX_SDK_DIR%\amxxmodule.cpp" (
	echo amxxmodule.cpp not found in AMXX_SDK_DIR="%AMXX_SDK_DIR%"
	exit /b 1
)

if not exist "%AMXX_PUBLIC_DIR%\IGameConfigs.h" (
	echo IGameConfigs.h not found in AMXX_PUBLIC_DIR="%AMXX_PUBLIC_DIR%"
	exit /b 1
)

where cl >nul 2>&1
if errorlevel 1 (
	echo cl.exe was not found. Use "x86 Native Tools Command Prompt for VS".
	exit /b 1
)

if not exist "%SCRIPT_DIR%build_win" mkdir "%SCRIPT_DIR%build_win"

cl /nologo /EHsc /O2 /std:c++17 /MT /LD /DHAVE_STDINT_H ^
	/I"%SCRIPT_DIR%src" ^
	/I"%AMXX_PUBLIC_DIR%" ^
	/I"%AMXX_SDK_DIR%" ^
	"%AMXX_SDK_DIR%\amxxmodule.cpp" ^
	"%SCRIPT_DIR%src\ebot_bridge.cpp" ^
	/link /OUT:"%SCRIPT_DIR%build_win\ebot_bridge_amxx.dll"

if errorlevel 1 exit /b 1

echo Built: %SCRIPT_DIR%build_win\ebot_bridge_amxx.dll
exit /b 0
