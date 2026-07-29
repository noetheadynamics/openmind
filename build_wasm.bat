@echo off
REM OpenMind WASM Build Script for Windows
REM Requires Emscripten SDK installed and activated

echo ========================================
echo  OpenMind WebAssembly Build
echo ========================================

REM Check for Emscripten
where emcc >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo ERROR: Emscripten not found in PATH.
    echo Please activate Emscripten SDK first:
    echo   emsdk activate latest
    echo.
    echo Or set EMSDK environment variable to your emsdk directory.
    goto :error
)

echo [1/4] Cleaning previous build...
if exist build_wasm rmdir /s /q build_wasm
mkdir build_wasm
cd build_wasm

echo [2/4] Configuring with CMake + Emscripten...
call emcmake cmake .. -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake configuration failed.
    cd ..
    goto :error
)

echo [3/4] Building WASM module...
call cmake --build . --config Release
if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed.
    cd ..
    goto :error
)

echo [4/4] Copying output files...
cd ..
copy build_wasm\openmind.js . >nul
copy build_wasm\openmind.wasm . >nul

echo.
echo ========================================
echo  BUILD SUCCESSFUL
echo ========================================
echo.
echo Output files:
if exist openmind.js   echo   openmind.js   - JS loader
if exist openmind.wasm echo   openmind.wasm - WASM binary
echo.
echo To test, open index.html in a browser.
echo.
goto :end

:error
echo.
echo Build failed. See errors above.
exit /b 1

:end
