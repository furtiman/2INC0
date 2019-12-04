#! /bin/sh

CFLAGS="-Wall -g -c -O2 -pthread"
LFLAGS="-lm -lpthread"

echo "compiling..."
gcc $CFLAGS threaded_basics.c || exit

echo "linking..."
gcc -o threaded threaded_basics.o $LFLAGS || exit
