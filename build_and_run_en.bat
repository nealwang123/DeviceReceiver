@echo off
echo Real-time Data Receiver Build Script
echo.

REM Set parameters
set BUILD_TYPE=Release
set CLEAN_BUILD=0
set RUN_AFTER=0

REM Parse arguments
:parse_args
if "%1"=="" goto :args_done
if /i "%1"=="-Debug" (
    set BUILD_TYPE=Debug
    shift
    goto :parse_args
)
if /i "%1"=="-Clean" (
    set CLEAN_BUILD=1
    shift
    goto :parse_args
)
if /i "%1"=="-Run" (
    set RUN_AFTER=1
    shift
    goto :parse_args
)
if /i "%1"=="-Help" (
    call :show_help
    exit /b 0
)
echo Unknown parameter: %1
call :show_help
exit /b 1

:show_help
echo Usage: build_and_run_en.bat [-Debug] [-Clean] [-Run] [-Help]
echo.
echo Parameters:
echo   -Debug    : Build debug version (default: release)
echo   -Clean    : Clean build files
echo   -Run      : Run program after successful build
echo   -Help     : Show this help message
echo.
echo Examples:
echo   build_and_run_en.bat           : Build release version
echo   build_and_run_en.bat -Debug    : Build debug version
echo   build_and_run_en.bat -Clean    : Clean build files
echo   build_and_run_en.bat -Run      : Build and run release version
echo   build_and_run_en.bat -Clean -Run : Clean, build and run
goto :eof

:args_done
echo Build type: %BUILD_TYPE%

REM 1. Check Visual Studio environment
echo.
echo [1/5] Setting up Visual Studio development environment...
set VCVARS="C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat"
if not exist %VCVARS% (
    echo Error: Cannot find vcvarsall.bat
    exit /b 1
)

call %VCVARS% x64
if errorlevel 1 (
    echo VS environment setup failed
    exit /b 1
)
echo VS environment setup successful

REM 2. Check Qt
echo.
echo [2/5] Checking Qt development environment...
set QMAKE="C:\Qt\5.15.2\msvc2019_64\bin\qmake.exe"
if not exist %QMAKE% (
    echo Error: Cannot find qmake.exe
    exit /b 1
)

REM 3. Clean build files (if specified)
if %CLEAN_BUILD%==1 (
    echo.
    echo [3/5] Cleaning build files...
    if exist build rmdir /s /q build
    if exist debug rmdir /s /q debug
    if exist release rmdir /s /q release
    if exist Makefile del /q Makefile 2>nul
    if exist Makefile.Debug del /q Makefile.Debug 2>nul
    if exist Makefile.Release del /q Makefile.Release 2>nul
    if exist .qmake.stash del /q .qmake.stash 2>nul
    echo Cleaning completed
)

REM 4. Run qmake
echo.
echo [4/5] Generating Makefile...
%QMAKE% realtime_data.pro
if errorlevel 1 (
    echo qmake failed
    exit /b 1
)
echo qmake successful

REM 5. Run nmake
echo.
echo [5/5] Compiling %BUILD_TYPE% version...
if "%BUILD_TYPE%"=="Release" (
    nmake release
) else (
    nmake debug
)

if errorlevel 1 (
    echo Compilation failed
    exit /b 1
)
echo Compilation successful

REM 6. Check executable file
echo.
echo Build completed!
if "%BUILD_TYPE%"=="Release" (
    set EXE_PATH=build\release\realtime_data.exe
) else (
    set EXE_PATH=build\debug\realtime_data.exe
)

if exist "%EXE_PATH%" (
    for %%F in ("%EXE_PATH%") do (
        set "FILE_SIZE=%%~zF"
    )
    REM Calculate file size (MB)
    set /a FILE_SIZE_MB=FILE_SIZE / 1048576
    set /a FILE_SIZE_KB=(FILE_SIZE %% 1048576) / 1024
    echo Executable file: %EXE_PATH%
    if %FILE_SIZE_MB% gtr 0 (
        echo File size: %FILE_SIZE_MB%.%FILE_SIZE_KB% MB
    ) else (
        set /a FILE_SIZE_KB=FILE_SIZE / 1024
        echo File size: %FILE_SIZE_KB% KB
    )
    
    REM 7. Run program (if specified)
    if %RUN_AFTER%==1 (
        echo.
        echo Starting program...
        start "" "%EXE_PATH%"
        echo Program started
    )
) else (
    echo Warning: Executable file not found: %EXE_PATH%
)

echo.
echo Build script execution completed
pause