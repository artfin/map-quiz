#/bin/sh/

CC=gcc

CFLAGS="-ggdb -Wall -Wextra"
INC="-I./raylib/include"
LIB="-L./raylib/lib -lraylib -lm -ldl -lpthread"

set -xe
$CC $CFLAGS $INC quiz.c -o quiz $LIB
