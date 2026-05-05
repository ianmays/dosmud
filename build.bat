@echo off
call \watcom\owsetenv

set LOG=build.log
set WFL=-bt=dos -I. -Iinclude -Isrc
if "%1"=="TEST_MODE" set WFL=%WFL% -DTEST_MODE

echo dosmud Open Watcom build log > %LOG%
echo. >> %LOG%
echo Date/time: %DATE% %TIME% >> %LOG%
echo WFL: %WFL% >> %LOG%
echo. >> %LOG%

REM COMMAND.COM limits one command line to about 127 characters. We compile each
REM .c to a .obj with a short line, then link in a second short wcl line.
REM (A single long wcl with all sources truncates; @rsp failed on some setups.)

if exist main.obj del main.obj
if exist game.obj del game.obj
if exist grendr.obj del grendr.obj
if exist invent.obj del invent.obj
if exist command.obj del command.obj
if exist world.obj del world.obj
if exist items.obj del items.obj
if exist dosmud.exe del dosmud.exe

echo Compiling main.c ... >> %LOG%
echo Compiling main.c ...
wcl %WFL% -c -fo=main.obj src\main.c >> %LOG%
if errorlevel 1 goto wcl_bad

echo Compiling game.c ... >> %LOG%
echo Compiling game.c ...
wcl %WFL% -c -fo=game.obj src\game.c >> %LOG%
if errorlevel 1 goto wcl_bad

echo Compiling grendr.c ... >> %LOG%
echo Compiling grendr.c ...
wcl %WFL% -c -fo=grendr.obj src\grendr.c >> %LOG%
if errorlevel 1 goto wcl_bad

echo Compiling invent.c ... >> %LOG%
echo Compiling invent.c ...
wcl %WFL% -c -fo=invent.obj src\invent.c >> %LOG%
if errorlevel 1 goto wcl_bad

echo Compiling command.c ... >> %LOG%
echo Compiling command.c ...
wcl %WFL% -c -fo=command.obj src\command.c >> %LOG%
if errorlevel 1 goto wcl_bad

echo Compiling world.c ... >> %LOG%
echo Compiling world.c ...
wcl %WFL% -c -fo=world.obj src\world.c >> %LOG%
if errorlevel 1 goto wcl_bad

echo Compiling items.c ... >> %LOG%
echo Compiling items.c ...
wcl %WFL% -c -fo=items.obj src\items.c >> %LOG%
if errorlevel 1 goto wcl_bad

echo Linking dosmud.exe ... >> %LOG%
echo Linking dosmud.exe ...
REM No -I here: link only; line stays under 127 chars.
wcl -bt=dos -fe=dosmud.exe main.obj game.obj grendr.obj invent.obj command.obj world.obj items.obj >> %LOG%
if errorlevel 1 goto wcl_bad

echo. >> %LOG%
echo wcl result: success ERRORLEVEL 0 >> %LOG%
echo Build OK. Created dosmud.exe
goto wcl_done

:wcl_bad
echo. >> %LOG%
echo wcl result: failure non-zero ERRORLEVEL >> %LOG%
echo Build FAILED. See %LOG%

:wcl_done
echo Full transcript: %LOG%
pause
