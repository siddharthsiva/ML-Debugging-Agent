@echo off
echo Building Mini-Snowflake...

REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake
echo Configuring with CMake...
cmake .. -DCMAKE_BUILD_TYPE=Release

REM Build the project
echo Building...
cmake --build . --config Release

echo Build complete! Run with: .\Release\mini_snowflake.exe