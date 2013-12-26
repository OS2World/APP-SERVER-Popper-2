#
#
#

OBJS=addit.o commands.o connect.o files.o main.o packmail.o passwd.o scanmail.o
CC=gcc
COMP=-c -Wall
LIBS=-lsocket
STRIP=emxbind -s
BINARY=popper.exe
REMOVE=rm -f


$(BINARY): $(OBJS)
	$(CC) $(OBJS) -o $(BINARY) $(LIBS)
	$(STRIP) $(BINARY)
.c.o:
	$(CC) $(COMP) $*.c
clean:
	$(REMOVE) $(OBJS) $(BINARY) core


