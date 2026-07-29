@echo off
echo Building OpenMind Voxel Engine...
mkdir build 2>nul
cd build
cmake .. -G "MinGW Makefiles" 2>nul || cmake .. -G "Visual Studio 17 2022" 2>nul || cmake .. -G "Visual Studio 16 2019" 2>nul
cmake --build . --config Release
if %ERRORLEVEL% EQU 0 (
    echo Build successful!
    echo Run: build\Release\OpenMind.exe
) else (
    echo Build failed.
)
cd ..
