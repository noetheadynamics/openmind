@echo off
REM ============================================================
REM OpenMind - Publish Desktop Update to GitHub Releases
REM Usage:  publish.bat <version>  (e.g. publish.bat 1.1.0)
REM Requires: gh CLI authenticated, Rust, Node
REM ============================================================
setlocal

if "%~1"=="" (
    echo Usage: publish.bat ^<version^>
    exit /b 1
)

set NEW_VERSION=%~1
set REPO=noetheadynamics/openmind

echo [1/4] Updating version to %NEW_VERSION% ...
node scripts/bump_version.js %NEW_VERSION%
if errorlevel 1 exit /b 1

echo [2/4] Building Tauri app ...
call npm run tauri:build
if errorlevel 1 exit /b 1

echo [3/4] Creating GitHub release v%NEW_VERSION% ...
gh release create "v%NEW_VERSION%"^
    "src-tauri/target/release/bundle/nsis/OpenMind_%NEW_VERSION%_x64-setup.exe"^
    "src-tauri/target/release/bundle/msi/OpenMind_%NEW_VERSION%_x64_en-US.msi"^
    --repo %REPO%^
    --title "OpenMind v%NEW_VERSION%"^
    --notes "OpenMind desktop update v%NEW_VERSION%"

if errorlevel 1 exit /b 1

echo.
echo ============================================================
echo Publish complete! Installed apps will auto-prompt to update.
echo ============================================================
endlocal
