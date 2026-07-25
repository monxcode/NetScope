@echo off
REM Build script for NetScope on Windows
REM Requirements: CMake 3.20+, Visual Studio 2022 (or MinGW-w64 with GCC 11+)

echo === NetScope Build Script ===
echo.

if "%1"=="" set CONFIG=Release
if not "%1"=="" set CONFIG=%1

echo Configuration: %CONFIG%
echo.

REM Remove old build directory
if exist build rmdir /s /q build

REM Configure
echo [1/3] Configuring CMake...
cmake -B build -DCMAKE_BUILD_TYPE=%CONFIG%
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

REM Build
echo [2/3] Building...
cmake --build build --config %CONFIG%
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%

REM Run tests
echo [3/3] Running tests...
ctest --test-dir build --output-on-failure -C %CONFIG%

echo.
echo === Build complete ===
echo Binary: build\bin\netscope.exe
