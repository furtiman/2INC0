#! /bin/sh

CFLAGS="-Wall -g -c"
LFLAGS="-lrt -lm"

echo "compiling interprocess basic..."
gcc $CFLAGS interprocess_basics.c || exit

echo "linking..."
gcc -o interprocess_basics interprocess_basics.o $LFLAGS || exit
