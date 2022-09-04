# Assignment 2 : Intro to System Programming
# Author: Swapnil Ghonge
# Description: This file contains Makefil

CC=${CROSS_COMPILE}gcc

CFLAGS=-Wall -Werror

all : writer

writer: writer.o
	${CC} ${CFLAGS}  writer.c -o writer

clean:
	-rm -f *.o writer
