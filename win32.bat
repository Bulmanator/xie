@echo off
setlocal

if not exist "build" (mkdir "build")

pushd "build" > nul

set compiler_flags=-nologo -I"..\runtime\include" -W4 -wd4201
set linker_flags=-incremental:no

rem build debug runtime .dll
rem
cl %compiler_flags% -I"..\thirdparty" -Od -Zi -LD "..\runtime\code\xi.c" -Fe"xid.dll" -link %linker_flags%

rem build opengl renderer .dll
rem
cl %compiler_flags% -Od -Zi -LD "..\renderer\xi_opengl.c" -Fe"xi_opengld.dll" -link %linker_flags%  -libpath:. xid.lib

rem build vulkan renderer .dll
rem IN TESTING!!
rem
cl %compiler_flags% -I%VULKAN_SDK%\Include -Od -Zi -LD "..\renderer\xi_vulkan.c" -Fe"xi_vulkan.dll" -link %linker_flags% -libpath:. -libpath:%VULKAN_SDK%\Lib xid.lib

rem @todo: build release versions of both runtime and renderer
rem

popd > nul
endlocal
