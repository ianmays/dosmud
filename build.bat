@echo off
call \watcom\owsetenv
wcl -bt=dos -I. -Iinclude -Isrc -fe=dosmud.exe src\main.c src\game.c src\command.c src\world.c
echo.
echo Compile finished. Press a key...
pause