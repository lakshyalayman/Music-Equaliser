#!/bin/bash
set -xe
CFLAGS="-Wall  -fsanitize=address -Wextra -O0 -g`pkg-config --cflags raylib`"
LIBS="`pkg-config --libs raylib` -lraylib -lglfw -lm -ldl -lpthread"

clang $CFLAGS -o libplug.so -fPIC -shared src/plug.c $LIBS
clang $CFLAGS -o musializer src/main.c $LIBS -L./ 
# LD_LIBRARY_PATH="./" ./musializer
