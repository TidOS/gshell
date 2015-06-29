# Ruby's magical, flexible makefile!
# Works with GNU Make.

RM = rm
CC = gcc


CFLAGS   = -g -Wall
INCFLAGS = -I . -I/usr/include/tcl8.4
LFLAGS   = -ltcl8.4

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
TARGET = gsh

all: $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCFLAGS) -c $< -o $@

clean:
	$(RM) $(TARGET) $(OBJS)
