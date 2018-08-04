CC=gcc
CFLAGS=-g -Wall
APUE=$(HOME)/Desktop/bmp/apue.3e
INC_PATH=-I$(APUE)/include
LIB_PATH=-L$(APUE)/lib
LIB=-lapue

read_all: read_all.c
	$(CC) $(CFLAGS) -c $@.c

bmp: bmp.c
	$(CC) $(CFLAGS) $(INC_PATH) $(LIB_PATH) $@.c read_all.o $(LIB) -o $@
	
all: read_all bmp

clean: 
	rm bmp read_all.o




