@echo off
setlocal

set EMSDK_DIR=C:\Users\USER\Documents\emsdk
set PROJECT_DIR=%~dp0..
set OUT_DIR=%PROJECT_DIR%\build\wasm

call "%EMSDK_DIR%\emsdk_env.bat" >nul 2>&1

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

echo === OpenMind WASM Build ===
echo Project: %PROJECT_DIR%
echo Output: %OUT_DIR%\openmind.js
echo.

set SRC_DIR=%PROJECT_DIR%\src

emcc -O3 -std=c++17 ^
    "%SRC_DIR%\engine\VoxelOctree.cpp" ^
    "%SRC_DIR%\engine\PhysicsEngine.cpp" ^
    "%SRC_DIR%\engine\openmind_engine.cpp" ^
    "%SRC_DIR%\llm\JSONValidator.cpp" ^
    "%SRC_DIR%\bridge\openmind_bridge.cpp" ^
    -I"%SRC_DIR%\engine" ^
    -I"%SRC_DIR%\llm" ^
    -I"%SRC_DIR%\agents" ^
    -I"%SRC_DIR%\bridge" ^
    -s EXPORTED_FUNCTIONS="['_initWorld','_setBlock','_setBlockSimple','_getBlockData','_removeBlock','_tickPhysics','_tickPhysicsDelta','_getWorldStats','_setTimeOfDay','_getTimeOfDay','_getSunlightIntensity','_setCycleDuration','_rewindTime','_setWeather','_getWeatherType','_getWeather','_setTimeScale','_getAgentCount','_getAgentData','_addAgent','_setAgentPosition','_exportCSV','_exportGLTF','_getLastExportCSV','_getLastExportGLTF','_saveSnapshot','_getPendingFragments','_setGravity','_setAmbientTemperature','_setWind','_cleanup','_getSunPosition','_generateFromPrompt','_teleportCamera','_setOverlay','_saveWorld','_malloc','_free']" ^
    -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap']" ^
    -s ALLOW_MEMORY_GROWTH=1 ^
    -s INITIAL_MEMORY=67108864 ^
    -s WASM=1 ^
    -s MODULARIZE=1 ^
    -s EXPORT_NAME="OpenMindModule" ^
    -s ENVIRONMENT="web" ^
    -s SINGLE_FILE=0 ^
    -o "%OUT_DIR%\openmind.js"

if %ERRORLEVEL% equ 0 (
    echo.
    echo Build SUCCESS!
    echo Output files:
    dir "%OUT_DIR%\openmind.*"
) else (
    echo.
    echo Build FAILED with error code %ERRORLEVEL%
)

endlocal
