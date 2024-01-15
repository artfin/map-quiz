#/bin/bash/

set -xe

CC=gcc
CFLAGS="-ggdb -Wall -Wextra"
INC="-I./raylib/include"
LIB="-L./raylib/lib -lraylib -lm -ldl -lpthread"

build_local() {
    $CC $CFLAGS $INC quiz.c -o quiz $LIB
}

build_raylib_for_web() {
    CC=emcc
    RAYLIB=./raylib-source-code/src
    GLFW_INC=-I./raylib-source-code/src/external/glfw/include

    mkdir -p build/
    $CC -c ${RAYLIB}/rcore.c -o build/rcore.o -Os -Wall -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
    $CC -c ${RAYLIB}/rshapes.c -o build/rshapes.o -Os -Wall -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
    $CC -c ${RAYLIB}/rtextures.c -o build/rtextures.o -Os -Wall -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
    $CC -c ${RAYLIB}/rtext.c -o build/rtext.o -Os -Wall -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
    $CC -c ${RAYLIB}/rmodels.c -o build/rmodels.o -Os -Wall -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
    $CC -c ${RAYLIB}/utils.c -o build/utils.o -Os -Wall -DPLATFORM_WEB
    $CC -c ${RAYLIB}/raudio.c -o build/raudio.o -Os -Wall -DPLATFORM_WEB 
    emar rcs build/libraylib.a build/rcore.o build/rshapes.o build/rtextures.o build/rtext.o build/rmodels.o build/utils.o build/raudio.o
}

build_for_web() {
    CC=emcc

    mkdir -p wasm
    $CC quiz.c -o wasm/quiz.html -Os -Wall --preload-file resources \
        build/libraylib.a $INC \
        -s USE_GLFW=3 \
        -s GL_ENABLE_GET_PROC_ADDRESS \
        -s TOTAL_MEMORY=67108864 \
        -s ALLOW_MEMORY_GROWTH=1 \
        --profiling \
        -DPLATFORM_WEB
}

#build_local

#build_raylib_for_web
build_for_web
