#! /bin/sh

CFLAGS="-Wall -g -c"
LFLAGS="-lrt -lm"

echo "compiling interprocess basic..."
gcc $CFLAGS interprocess_basics.c || exit
echo "compiling worker..."
gcc $CFLAGS worker.c || exit

echo "linking interprocess basic..."
gcc -o interprocess_basics interprocess_basics.o $LFLAGS || exit
echo "linking worker..."
gcc -o worker worker.o $LFLAGS || exit