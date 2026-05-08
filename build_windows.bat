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
    for /f "tokens=*" %%P in ('where qmake 2^>nul') do if not defined QMAKE_FULL set "QMAKE_FULL=%%P"
    if defined QMAKE_FULL set "QMAKE=!QMAKE_FULL!"
    echo Found qmake in PATH: !QMAKE!
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
:: Ensure QTBIN is always set (needed for windeployqt later).
:: The most reliable method is to ask qmake itself.
for /f "tokens=*" %%B in ('"%QMAKE%" -query QT_INSTALL_BINS 2^>nul') do set "QTBIN=%%B"
if not defined QTBIN (
    for %%Q in ("!QMAKE!") do set "QTBIN=%%~dpQ"
    if "!QTBIN:~-1!"=="\" set "QTBIN=!QTBIN:~0,-1!"
)
echo Qt bin dir: !QTBIN!

"%QMAKE%" -query QT_VERSION >nul 2>&1
if !ERRORLEVEL! neq 0 (
    echo ERROR: qmake found but not working. Check your QTDIR path.
    pause
    exit /b 1
)
for /f "tokens=*" %%V in ('"%QMAKE%" -query QT_VERSION 2^>nul') do echo Qt version: %%V

:: ---- Detect build tool based on Qt kit type --------------------
:: Determine kit type from the qmake path (msvc vs mingw) and the
:: target architecture (_64 vs _32). We MUST set up the build tool
:: that matches the Qt kit, otherwise we get 32-vs-64-bit mismatches
:: at link time.
set "KIT_TYPE="
echo !QMAKE! | findstr /i "msvc"  >nul 2>&1 && set "KIT_TYPE=msvc"
echo !QMAKE! | findstr /i "mingw" >nul 2>&1 && set "KIT_TYPE=mingw"

set "VC_ARCH=x86"
echo !QMAKE! | findstr /i "_64" >nul 2>&1 && set "VC_ARCH=amd64"

echo Kit type: !KIT_TYPE!  /  Architecture: !VC_ARCH!

set "MAKE="

if "!KIT_TYPE!"=="msvc" (
    :: Always call vcvarsall with the matching arch -- don't trust an
    :: nmake that might already be in PATH from a 32-bit install.
    set "VCVARS="
    for %%Y in (2022 2019 2017) do (
        for %%E in (Community Professional Enterprise BuildTools) do (
            set "_v=%ProgramFiles%\Microsoft Visual Studio\%%Y\%%E\VC\Auxiliary\Build\vcvarsall.bat"
            if exist "!_v!" if not defined VCVARS set "VCVARS=!_v!"
            set "_v=%ProgramFiles(x86)%\Microsoft Visual Studio\%%Y\%%E\VC\Auxiliary\Build\vcvarsall.bat"
            if exist "!_v!" if not defined VCVARS set "VCVARS=!_v!"
        )
    )
    if defined VCVARS (
        echo Setting up MSVC environment via:
        echo   !VCVARS! !VC_ARCH!
        call "!VCVARS!" !VC_ARCH!
        where nmake >nul 2>&1 && set "MAKE=nmake"
    ) else (
        echo.
        echo ERROR: Visual Studio Build Tools not found, but the Qt kit
        echo is MSVC. Install VS Build Tools 2022:
        echo   https://aka.ms/vs/17/release/vs_BuildTools.exe
        echo   Tick "Desktop development with C++" then re-run this script.
        echo.
        pause
        exit /b 1
    )
) else if "!KIT_TYPE!"=="mingw" (
    :: MinGW Qt kit -- find mingw32-make
    where mingw32-make >nul 2>&1 && set "MAKE=mingw32-make"
    if not defined MAKE (
        for %%G in ("C:\Qt\Tools\llvm-mingw1706_64\bin") do (
            if exist "%%~G\mingw32-make.exe" if not defined MAKE (
                set "PATH=%%~G;!PATH!"
                set "MAKE=mingw32-make"
                echo Found MinGW at %%~G
            )
        )
    )
    if not defined MAKE (
        for /d %%G in ("C:\Qt\Tools\mingw*" "C:\Qt\Tools\llvm-mingw*") do (
            if exist "%%G\bin\mingw32-make.exe" if not defined MAKE (
                set "PATH=%%G\bin;!PATH!"
                set "MAKE=mingw32-make"
                echo Found MinGW at %%G\bin
            )
        )
    )
) else (
    echo.
    echo WARNING: Could not determine kit type from QMAKE path.
    echo Falling back to whatever build tool is on PATH.
    where nmake >nul 2>&1        && set "MAKE=nmake"
    where mingw32-make >nul 2>&1 && if not defined MAKE set "MAKE=mingw32-make"
)

if not defined MAKE (
    echo.
    echo ERROR: No suitable C++ build tool found for kit type !KIT_TYPE!.
    echo.
    echo   For MSVC kit -- Install VS Build Tools 2022:
    echo     https://aka.ms/vs/17/release/vs_BuildTools.exe
    echo     Tick "Desktop development with C++"
    echo.
    echo   For MinGW kit -- Install via Qt Maintenance Tool:
    echo     Open C:\Qt\MaintenanceTool.exe
    echo     Add Qt 6.x MinGW kit and update QTDIR at top of this script.
    echo.
    pause
    exit /b 1
)
echo Build tool: !MAKE!
echo.

:: ---- Verify cl.exe is reachable for MSVC kits --------------------
:: Echo strings here use ^( and ^) where parens appear, otherwise the
:: cmd parser would close the surrounding if-block prematurely.
if /i "!KIT_TYPE!"=="msvc" (
    where cl >nul 2>&1
    if !ERRORLEVEL! neq 0 (
        echo.
        echo ERROR: cl.exe still not in PATH after vcvarsall setup.
        echo This usually means the VS Build Tools install is incomplete.
        echo Re-run the VS installer and tick "Desktop development with C++".
        echo.
        pause
        exit /b 1
    )
    echo Compiler: cl.exe OK
)

:: ---- Paths -------------------------------------------------------
set "PROJDIR=%~dp0"
:: Strip trailing backslash
if "%PROJDIR:~-1%"=="\" set "PROJDIR=%PROJDIR:~0,-1%"

:: Check for characters that break nmake/qmake path handling.
:: Spaces alone are usually fine; parentheses are not -- nmake treats
:: them as group operators so a path like "...(1)\..." causes a parse
:: error at link time even if compilation succeeded.
echo !PROJDIR! | findstr /r "[()]" >nul 2>&1
if !ERRORLEVEL!==0 (
    echo.
    echo ERROR: Project path contains parentheses:
    echo   !PROJDIR!
    echo.
    echo nmake treats ^( and ^) as special syntax, so the build will fail
    echo during the link step even if compilation appears to succeed.
    echo.
    echo Fix: move the project to a path with no spaces or parentheses.
    echo Example: C:\Projects\GridSim\
    echo.
    echo Then run this script from the new location.
    echo.
    pause
    exit /b 1
)

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
set "LOGFILE=%BUILDDIR%\build.log"
echo.
echo Building (this may take a minute)...
echo Full build output will be saved to: !LOGFILE!
echo.

if "!MAKE!"=="nmake" (
    nmake release > "!LOGFILE!" 2>&1
) else (
    !MAKE! > "!LOGFILE!" 2>&1
)
set "BUILD_ERR=!ERRORLEVEL!"

:: Always print the full log so the real error is visible in the window
type "!LOGFILE!"

if !BUILD_ERR! neq 0 (
    echo.
    echo ==================================================
    echo  Build FAILED.
    echo  The compiler error is shown above ^(scroll up^).
    echo  Full log also saved to:
    echo    !LOGFILE!
    echo  Open it in Notepad for easier reading:
    echo    notepad "!LOGFILE!"
    echo ==================================================
    echo.
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

:: Use absolute path so windeployqt is found even if vcvarsall reset PATH
set "WINDEPLOYQT="
for %%N in (windeployqt.exe windeployqt6.exe) do (
    if not defined WINDEPLOYQT if exist "!QTBIN!\%%N" set "WINDEPLOYQT=!QTBIN!\%%N"
)
:: Last resort: check PATH
if not defined WINDEPLOYQT (
    for %%N in (windeployqt.exe windeployqt6.exe) do (
        if not defined WINDEPLOYQT (
            where %%N >nul 2>&1 && for /f "tokens=*" %%P in ('where %%N 2^>nul') do (
                if not defined WINDEPLOYQT set "WINDEPLOYQT=%%P"
            )
        )
    )
)
if not defined WINDEPLOYQT (
    echo.
    echo ==================================================
    echo  BUILD OK but windeployqt NOT FOUND
    echo  Qt bin dir searched: !QTBIN!
    echo  The .exe has been copied but Qt DLLs are missing.
    echo  Find windeployqt manually and run:
    echo    windeployqt --release "%DEPLOYDIR%\GridSimUserControl.exe"
    echo ==================================================
    echo.
    pause
    exit /b 1
)

echo Running windeployqt...
echo   !WINDEPLOYQT!
"!WINDEPLOYQT!" ^
    --release ^
    --no-translations ^
    --compiler-runtime ^
    "%DEPLOYDIR%\GridSimUserControl.exe"
if !ERRORLEVEL! neq 0 (
    echo ERROR: windeployqt failed.
    pause
    exit /b 1
)
echo windeployqt OK

:: Qt SerialPort plugin is sometimes missed -- ensure it is present
for /f "tokens=*" %%P in ('"!QTBIN!\qmake.exe" -query QT_INSTALL_PLUGINS 2^>nul') do set "QTPLUGINS=%%P"
if defined QTPLUGINS (
    if exist "!QTPLUGINS!\serialport" (
        if not exist "%DEPLOYDIR%\serialport" (
            xcopy /e /i /q "!QTPLUGINS!\serialport" "%DEPLOYDIR%\serialport\" >nul
            echo Copied serialport plugin
        ) else (
            echo serialport plugin already present
        )
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
