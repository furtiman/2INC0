#! /bin/sh

CFLAGS="-Wall -g -c -O2 -pthread"
LFLAGS="-lm -lpthread"

echo "compiling flip..."
gcc $CFLAGS flip.c || exit

echo "linking flip..."
gcc -o flip flip.o $LFLAGS || exit
