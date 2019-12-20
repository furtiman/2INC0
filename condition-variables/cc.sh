#! /bin/sh

CFLAGS="-Wall -g -c -O2 -pthread"
LFLAGS="-lm -lpthread"

echo "compiling..."
gcc $CFLAGS prodcons.c || exit
# gcc $CFLAGS thread_pool.c || exit

echo "linking..."
gcc -o prodcons prodcons.o $LFLAGS || exit #  thread_pool.o
