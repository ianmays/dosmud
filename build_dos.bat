@echo off
REM Open Watcom: run owsetenv.bat first (or use the Open Watcom Command Prompt).
setlocal
cd /d "%~dp0"
wcl -q -bt=dos -ms -0 -s -w3 -Iinclude -Isrc -fe=dosmud.exe src\main.c src\game.c src\command.c src\world.c
exit /b %ERRORLEVEL%
