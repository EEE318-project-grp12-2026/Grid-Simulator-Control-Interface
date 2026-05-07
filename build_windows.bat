@echo off
setlocal EnableDelayedExpansion

echo ==================================================
echo  Grid Simulator -- Windows portable build
echo ==================================================
echo.

:: ------------------------------------------------------------------
:: OPTIONAL: Set QTDIR to your Qt kit folder if qmake is not in PATH.
:: Example:  set "QTDIR=C:\Qt\6.5.3\msvc2022_64"
::           set "QTDIR=C:\Qt\5.15.2\mingw81_64"
:: Leave blank to auto-detect.
:: ------------------------------------------------------------------
set "QTDIR="

:: ---- Locate qmake ------------------------------------------------
set "QMAKE=qmake"

if defined QTDIR (
    set "PATH=%QTDIR%\bin;%PATH%"
    set "QMAKE=%QTDIR%\bin\qmake.exe"
    echo Using QTDIR: %QTDIR%
    goto :found_qt
)

where qmake >nul 2>&1
if !ERRORLEVEL!==0 (
    echo Found qmake in PATH
    goto :found_qt
)

echo qmake not in PATH, scanning common Qt install locations...
for /d %%V in ("C:\Qt\6.*" "C:\Qt\5.*") do (
    for /d %%K in ("%%V\msvc2022_64" "%%V\msvc2019_64" "%%V\mingw*") do (
        if exist "%%K\bin\qmake.exe" (
            if not defined QTBIN set "QTBIN=%%K\bin"
        )
    )
)
if defined QTBIN (
    set "PATH=!QTBIN!;%PATH%"
    set "QMAKE=!QTBIN!\qmake.exe"
    echo Found Qt at !QTBIN!
    goto :found_qt
)

echo.
echo ERROR: qmake not found. Try one of:
echo   1. Run this script from Qt Creator's "Qt Command Prompt"
echo      (Tools -^> External -^> Open Terminal)
echo   2. Set QTDIR at the top of this script
echo   3. Add your Qt kit's bin\ folder to your PATH
echo.
pause
exit /b 1

:found_qt
"%QMAKE%" -query QT_VERSION >nul 2>&1
if !ERRORLEVEL! neq 0 (
    echo ERROR: qmake found but not working. Check your QTDIR path.
    pause
    exit /b 1
)
for /f "tokens=*" %%V in ('"%QMAKE%" -query QT_VERSION 2^>nul') do echo Qt version: %%V

:: ---- Detect build tool -------------------------------------------
set "MAKE="
where nmake >nul 2>&1        && set "MAKE=nmake"
where mingw32-make >nul 2>&1 && if not defined MAKE set "MAKE=mingw32-make"
where make >nul 2>&1         && if not defined MAKE set "MAKE=make"

if not defined MAKE (
    echo.
    echo ERROR: No build tool found.
    echo   For MSVC builds: run from a "Developer Command Prompt for VS"
    echo   For MinGW builds: add MinGW bin\ to PATH
    echo.
    pause
    exit /b 1
)
echo Build tool: %MAKE%
echo.

:: ---- Paths -------------------------------------------------------
set "PROJDIR=%~dp0"
:: Strip trailing backslash
if "%PROJDIR:~-1%"=="\" set "PROJDIR=%PROJDIR:~0,-1%"

set "BUILDDIR=%PROJDIR%\build_release"
set "DEPLOYDIR=%PROJDIR%\dist_windows"

echo Cleaning previous builds...
if exist "%BUILDDIR%" rd /s /q "%BUILDDIR%"
if exist "%DEPLOYDIR%" rd /s /q "%DEPLOYDIR%"
mkdir "%BUILDDIR%"
mkdir "%DEPLOYDIR%"

:: ---- qmake -------------------------------------------------------
echo Running qmake...
cd /d "%BUILDDIR%"
"%QMAKE%" "%PROJDIR%\GridSimUserControl.pro" CONFIG+=release
if !ERRORLEVEL! neq 0 (
    echo ERROR: qmake failed. Check for .pro file errors.
    cd /d "%PROJDIR%"
    pause
    exit /b 1
)

:: ---- Build -------------------------------------------------------
echo.
echo Building (this may take a minute)...
if "%MAKE%"=="nmake" (
    nmake release
) else (
    %MAKE%
)
if !ERRORLEVEL! neq 0 (
    echo ERROR: Build failed. Check the output above for compiler errors.
    cd /d "%PROJDIR%"
    pause
    exit /b 1
)
cd /d "%PROJDIR%"

:: ---- Find the .exe -----------------------------------------------
set "EXEFILE="
for %%F in ("%BUILDDIR%\release\GridSimUserControl.exe"
            "%BUILDDIR%\GridSimUserControl.exe") do (
    if exist "%%F" if not defined EXEFILE set "EXEFILE=%%F"
)
if not defined EXEFILE (
    echo ERROR: Could not find built .exe in %BUILDDIR%
    echo Searching recursively...
    for /r "%BUILDDIR%" %%F in (GridSimUserControl.exe) do (
        if not defined EXEFILE set "EXEFILE=%%F"
    )
)
if not defined EXEFILE (
    echo ERROR: GridSimUserControl.exe not found anywhere in %BUILDDIR%
    pause
    exit /b 1
)
echo Built: %EXEFILE%

:: ---- Copy to deploy folder and run windeployqt -------------------
echo.
echo Copying to deploy folder...
copy "%EXEFILE%" "%DEPLOYDIR%\" >nul

echo Running windeployqt...
where windeployqt >nul 2>&1
if !ERRORLEVEL! neq 0 (
    echo.
    echo WARNING: windeployqt not found in PATH.
    echo The .exe has been copied but Qt DLLs are not bundled.
    echo Run windeployqt manually on: %DEPLOYDIR%\GridSimUserControl.exe
    echo.
    goto :done
)

windeployqt --release --no-translations --no-opengl-sw "%DEPLOYDIR%\GridSimUserControl.exe"
if !ERRORLEVEL! neq 0 (
    echo ERROR: windeployqt failed.
    pause
    exit /b 1
)

:: Qt SerialPort plugin isn't always caught by windeployqt -- ensure it's there
for /f "tokens=*" %%P in ('"%QMAKE%" -query QT_INSTALL_PLUGINS 2^>nul') do set "QTPLUGINS=%%P"
if defined QTPLUGINS (
    if exist "%QTPLUGINS%\serialport" (
        xcopy /e /i /q "%QTPLUGINS%\serialport" "%DEPLOYDIR%\serialport\" >nul
        echo Copied serialport plugin
    )
)

:done
echo.
echo ==================================================
echo  SUCCESS
echo  Portable folder: %DEPLOYDIR%\
echo  Copy the entire folder to any Windows PC to run.
echo  (No Qt install needed on the target machine.)
echo ==================================================
echo.
pause
