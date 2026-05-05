@echo off
call \watcom\owsetenv

set FLAGS=-bt=dos -I. -Iinclude -Isrc

if "%1"=="TEST_MODE" goto test

goto build

:test
echo TEST MODE
set FLAGS=%FLAGS% -DTEST_MODE

:build
wcl %FLAGS% -fe=dosmud.exe src\*.c

echo.
echo Compile finished. Press a key...
pause