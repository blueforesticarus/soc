PROGS =	pty
OBJS = lib.o
CFLAGS = -g -Wall -static

all:	pty soc

CC = musl-gcc -static

pty:	$(OBJS) pty.o
	$(CC) -Wall -g -o pty $(OBJS) pty.o

soc:	$(OBJS) soc.o
	$(CC) -Wall -g -o soc $(OBJS) soc.o

clean:
	rm -f pty soc *.o
