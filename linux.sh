#!/bin/sh

if [[ ! -d "build" ]];
then
    mkdir "build"
fi

pushd "build" > /dev/null

compiler_flags="-I../runtime/include -fPIC -Wall -Wno-unused-function -Wno-missing-braces"
linker_flags=""

# build debug runtime .so
#
gcc $compiler_flags -I"../thirdparty" -O0 -g -ggdb -shared "../runtime/code/xi.c" -o "libxid.so" $linker_flags

# build opengl renderer .so
#
gcc $compiler_flags -O0 -g -ggdb -shared "../renderer/xi_opengl.c" -o "xi_opengld.so" $linker_flags -lGL

popd > /dev/null
