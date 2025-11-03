@echo off
REM Windows launcher with auto-install for T4-S3 Simulator
REM For total beginners - handles MSYS2 installation automatically

setlocal enabledelayedexpansion

REM ============================================
REM Set MSYS2 paths
REM ============================================
set MSYS2_PATH=C:\tools\msys64
set BASH_EXE=%MSYS2_PATH%\usr\bin\bash.exe
set MSYSTEM=MINGW64
set CHERE_INVOKING=1
set SCRIPT_DIR=%~dp0
set SCRIPT_DIR=%SCRIPT_DIR:~0,-1%

REM ============================================
REM Main Menu
REM ============================================
:menu
echo.
echo ======================================
echo   T4-S3 Simulator - Menu
echo ======================================
echo 1. Build
echo 2. Run
echo 3. Install Requirements
echo 4. Exit
echo ======================================
set /p choice="Enter your choice [1-4]: "

if "%choice%"=="1" goto build
if "%choice%"=="2" goto run
if "%choice%"=="3" goto install
if "%choice%"=="4" goto exit
echo Invalid option. Please choose 1, 2, 3, or 4.
goto menu

REM ============================================
REM Build
REM ============================================
:build
echo.
echo ======================================
echo   Building...
echo ======================================

REM Check if MSYS2 exists before building
if not exist "%BASH_EXE%" (
    echo ERROR: MSYS2 not found!
    echo Please install MSYS2 first using option 3.
    echo.
    pause
    goto menu
)

"%BASH_EXE%" -leo pipefail -c "cd '%SCRIPT_DIR%/simulator' && ./build.sh clean"
if errorlevel 1 (
    echo.
    echo Build failed! You may need to install requirements first.
    echo Try option 3 to install requirements.
    pause
)
goto menu

REM ============================================
REM Run
REM ============================================
:run
echo.
echo ======================================
echo   Running...
echo ======================================

REM Check if MSYS2 exists before running
if not exist "%BASH_EXE%" (
    echo ERROR: MSYS2 not found!
    echo Please install MSYS2 first using option 3.
    echo.
    pause
    goto menu
)

"%BASH_EXE%" -leo pipefail -c "cd '%SCRIPT_DIR%/simulator' && ./run.sh"
if errorlevel 1 (
    echo.
    echo Run failed! Did you build the project first?
    pause
)
goto menu

REM ============================================
REM Install Requirements
REM ============================================
:install
echo ======================================
echo   Installing Requirements
echo ======================================
echo.

REM Check if MSYS2 is installed
if not exist "%BASH_EXE%" (
    echo Installing MSYS2...
    call :install_msys2
    if errorlevel 1 (
        echo MSYS2 installation failed!
        pause
        goto menu
    )
)

REM Install/update packages automatically
echo Installing build packages...
echo.

"%BASH_EXE%" -leo pipefail -c "pacman -Syu --needed --noconfirm mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_image mingw-w64-x86_64-curl"

echo.
echo ======================================
echo   Installation Complete!
echo ======================================

echo.
echo ======================================
pause
goto menu

REM ============================================
REM Exit
REM ============================================
:exit
echo Exiting...
exit /b 0

REM ============================================
REM Install MSYS2 Function
REM ============================================
:install_msys2
echo.
echo ======================================
echo   Installing MSYS2
echo ======================================
echo.


REM Create tools directory
if not exist "C:\tools" mkdir "C:\tools"

REM Download MSYS2 installer
echo [1/4] Downloading MSYS2 installer...
set INSTALLER=msys2-x86_64-latest.exe
set URL=https://github.com/msys2/msys2-installer/releases/download/nightly-x86_64/msys2-x86_64-latest.exe

powershell -Command "& {[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri '%URL%' -OutFile '%TEMP%\%INSTALLER%'}"

if not exist "%TEMP%\%INSTALLER%" (
    echo Failed to download MSYS2 installer.
    exit /b 1
)

REM Run installer silently
echo [2/4] Installing MSYS2 to C:\tools\msys64...
echo This may take a few minutes...
"%TEMP%\%INSTALLER%" install --root C:\tools\msys64 --confirm-command

if errorlevel 1 (
    echo Installation failed.
    echo Trying alternative method...
    "%TEMP%\%INSTALLER%" in --root=C:\tools\msys64
    if errorlevel 1 (
        echo Alternative installation also failed.
        del "%TEMP%\%INSTALLER%"
        exit /b 1
    )
)

REM Clean up installer
del "%TEMP%\%INSTALLER%"

REM Initialize MSYS2
echo [3/4] Initializing MSYS2...
C:\tools\msys64\usr\bin\bash.exe -lc "pacman -Syu --noconfirm"

REM Update package database
echo [4/4] Updating package database...
C:\tools\msys64\usr\bin\bash.exe -lc "pacman -Sy --noconfirm"

echo.
echo ======================================
echo   MSYS2 Installation Complete!
echo ======================================
echo.
pause

exit /b 0
