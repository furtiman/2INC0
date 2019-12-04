#! /bin/sh

CFLAGS="-Wall -g -c -O2 -pthread"
LFLAGS="-lm -lpthread"

echo "compiling threaded basics..."
gcc $CFLAGS threaded_basics.c || exit

echo "linking threaded basics..."
gcc -o threaded threaded_basics.o $LFLAGS || exit
