#/bin/sh/

CC=gcc

CFLAGS="-ggdb -Wall"
INC="-I./raylib/include"
LIB="-L./raylib/lib -lraylib -lm"

set -xe
$CC $CFLAGS $INC quiz.c -o quiz $LIB
