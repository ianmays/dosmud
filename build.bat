@echo off
call \watcom\owsetenv

set FLAGS=-bt=dos -I. -Iinclude -Isrc
set LOG=build.log

if "%1"=="TEST_MODE" goto test

goto start_log

:test
echo TEST MODE
set FLAGS=%FLAGS% -DTEST_MODE

:start_log
echo dosmud Open Watcom build log > %LOG%
echo. >> %LOG%
echo Date/time: %DATE% %TIME% >> %LOG%
echo FLAGS: %FLAGS% >> %LOG%
echo Command: wcl %FLAGS% -fe=dosmud.exe src\main.c src\game.c src\invent.c src\command.c src\world.c src\items.c >> %LOG%
echo. >> %LOG%

echo Running wcl... normal compiler output appends to %LOG%
REM No stderr redirect: COMMAND.COM can pass the digit 2 to wcl as a file name.
REM Result would be Unable to open 2 dot c. Stdout only below.
wcl %FLAGS% -fe=dosmud.exe src\main.c src\game.c src\invent.c src\command.c src\world.c src\items.c >> %LOG%

REM Test ERRORLEVEL immediately (before any other command clears it).
REM MS-DOS: IF ERRORLEVEL n is true if exit code is >= n.
if errorlevel 1 goto wcl_bad

echo. >> %LOG%
echo wcl result: success ERRORLEVEL 0 >> %LOG%
echo Build OK.
goto wcl_done

:wcl_bad
echo. >> %LOG%
echo wcl result: failure non-zero ERRORLEVEL >> %LOG%
echo Build FAILED.

:wcl_done
echo Full transcript: %LOG%
pause
