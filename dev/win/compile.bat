@echo off
set SOURCE_DIR=C:\Users\cdeshotel\source\repos\LiveImproved
set BUILD_DIR=%SOURCE_DIR%\build\win
set OUTPUT_DIR=%BUILD_DIR%
set LOG_FILE=%OUTPUT_DIR%\compile_log.txt
set FLAG_FILE=%OUTPUT_DIR%\compilation_complete.flag

REM Set full paths to CMake, Ninja, and VS2022 Clang
set CMAKE_PATH="C:\Program Files\CMake\bin"
set NINJA_PATH="C:\Program Files\Ninja"
set VS2022_CLANG_PATH="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\bin"

set PATH=%VS2022_CLANG_PATH%;%CMAKE_PATH%;%NINJA_PATH%;%PATH%

echo Starting CMake and Ninja build process...
echo "Source directory: %SOURCE_DIR%"
echo "Build directory: %BUILD_DIR%"
echo "Output directory: %OUTPUT_DIR%"
echo "Current PATH: %PATH%"

if exist %FLAG_FILE% (
    del %FLAG_FILE%
    echo Removed old flag file.
)

if not exist %BUILD_DIR% (
    mkdir %BUILD_DIR%
    echo Created build dir.
)

cd %SOURCE_DIR%

REM Generate Ninja build files with CMake
cmake -G "Ninja" -B"%BUILD_DIR%" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .
REM or call with --debug-output
if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed.
    exit /b %ERRORLEVEL%
)

REM Build the project with Ninja
ninja -C %BUILD_DIR%
if %ERRORLEVEL% NEQ 0 (
    echo Ninja build failed.
    exit /b %ERRORLEVEL%
)

echo Build finished at %date% %time%

echo Build process completed.
echo Creating completion flag file...
echo Build completed > %FLAG_FILE%

echo Windows script execution complete.
