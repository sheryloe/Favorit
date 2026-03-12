@echo off
setlocal
cmake -S "%~dp0" -B "%~dp0build" -G "MinGW Makefiles"
if errorlevel 1 exit /b 1
cmake --build "%~dp0build"
if errorlevel 1 exit /b 1
"%~dp0build\favorite_widget.exe"
