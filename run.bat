@echo off
cd /d "%~dp0"
echo Building Smart Maze Solver C++ backend...
g++ -std=c++17 -O2 -Wall -Wextra -o maze_server.exe backend\main.cpp -lws2_32
if errorlevel 1 (
  echo Build failed. Make sure MinGW g++ is installed and available in PATH.
  pause
  exit /b 1
)
echo Starting at http://localhost:8080
maze_server.exe
