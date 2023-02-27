@echo off
setlocal

if not exist "build" (mkdir "build")

pushd "build" > nul

set compiler_flags=-nologo -I"..\runtime\include" -W4 -wd4201
set linker_flags=-incremental:no

rem build debug runtime .dll
rem
cl %compiler_flags% -Od -Zi -LD "..\runtime\code\xi.c" -Fe"xid.dll" -link %linker_flags%

rem build debug engine .exe
rem
cl %compiler_flags% -Od -Zi "..\engine\win32_xie.c" -Fe"xie_debug.exe" -link %linker_flags%

rem @todo: build release versions of both runtime and engine
rem

popd > nul
endlocal
