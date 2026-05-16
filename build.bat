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
REM .c to a .obj with a short line, pack gameplay objects into gameplay.lib (wlib),
REM then link with one short wcl line (same shape as before the game.c split).

if exist main.obj del main.obj
if exist game.obj del game.obj
if exist gprog.obj del gprog.obj
if exist combat.obj del combat.obj
if exist genc.obj del genc.obj
if exist wanderer.obj del wanderer.obj
if exist dialogue.obj del dialogue.obj
if exist gatmos.obj del gatmos.obj
if exist grendr.obj del grendr.obj
if exist invent.obj del invent.obj
if exist command.obj del command.obj
if exist world.obj del world.obj
if exist items.obj del items.obj
if exist txtres.obj del txtres.obj
if exist platdos.obj del platdos.obj
if exist gameplay.lib del gameplay.lib
if exist dosmud.exe del dosmud.exe

echo Compiling main.c ... >> %LOG%
echo Compiling main.c ...
wcl %WFL% -c -fo=main.obj src\main.c >> %LOG%
if errorlevel 1 goto wcl_bad

echo Compiling platdos.c ... >> %LOG%
echo Compiling platdos.c ...
wcl %WFL% -c -fo=platdos.obj src\platdos.c >> %LOG%
if errorlevel 1 goto wcl_bad

echo Compiling game.c ... >> %LOG%
echo Compiling game.c ...
wcl %WFL% -c -fo=game.obj src\game.c >> %LOG%
if errorlevel 1 goto wcl_bad

echo Compiling gprog.c ... >> %LOG%
echo Compiling gprog.c ...
wcl %WFL% -c -fo=gprog.obj src\gprog.c >> %LOG%
if errorlevel 1 goto wcl_bad

echo Compiling combat.c ... >> %LOG%
echo Compiling combat.c ...
wcl %WFL% -c -fo=combat.obj src\combat.c >> %LOG%
if errorlevel 1 goto wcl_bad

echo Compiling genc.c ... >> %LOG%
echo Compiling genc.c ...
wcl %WFL% -c -fo=genc.obj src\genc.c >> %LOG%
if errorlevel 1 goto wcl_bad

echo Compiling wanderer.c ... >> %LOG%
echo Compiling wanderer.c ...
wcl %WFL% -c -fo=wanderer.obj src\wanderer.c >> %LOG%
if errorlevel 1 goto wcl_bad

echo Compiling dialogue.c ... >> %LOG%
echo Compiling dialogue.c ...
wcl %WFL% -c -fo=dialogue.obj src\dialogue.c >> %LOG%
if errorlevel 1 goto wcl_bad

echo Compiling gatmos.c ... >> %LOG%
echo Compiling gatmos.c ...
wcl %WFL% -c -fo=gatmos.obj src\gatmos.c >> %LOG%
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

echo Compiling txtres.c ... >> %LOG%
echo Compiling txtres.c ...
wcl %WFL% -c -fo=txtres.obj src\txtres.c >> %LOG%
if errorlevel 1 goto wcl_bad

echo Archiving gameplay.lib ... >> %LOG%
echo Archiving gameplay.lib ...
wlib -n gameplay.lib +game.obj +gprog.obj +combat.obj +genc.obj +wanderer.obj +dialogue.obj +gatmos.obj >> %LOG%
if errorlevel 1 goto wcl_bad

echo Linking dosmud.exe ... >> %LOG%
echo Linking dosmud.exe ...
REM Link line below is ~125 chars (under COMMAND.COM ~127); platdos.obj fits without platform.lib.
wcl -bt=dos -fe=dosmud.exe main.obj platdos.obj gameplay.lib grendr.obj invent.obj command.obj world.obj items.obj txtres.obj >> %LOG%
if errorlevel 1 goto wcl_bad
if not exist dosmud.exe goto wcl_bad

echo. >> %LOG%
echo wcl result: success ERRORLEVEL 0 >> %LOG%
echo Build OK. Created dosmud.exe
goto wcl_done

:wcl_bad
echo. >> %LOG%
echo wcl/wlib result: failure (non-zero ERRORLEVEL or missing dosmud.exe) >> %LOG%
echo Build FAILED. See %LOG%

:wcl_done
echo Full transcript: %LOG%