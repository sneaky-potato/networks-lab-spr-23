# Makefile variables
CC = gcc
CFLAGS = -Wall
RM = rm -f
TARGET_LIB = libmsocket.a
MAIN = mysocket.c

OBJS = $(MAIN:.c=.o)
HEADERS = $(MAIN:.c=.h)

$(TARGET_LIB): $(OBJS)
	$(AR) rcs $@ $^

$(OBJS): $(HEADERS)
	$(CC) $(CFLAGS) -c $(MAIN)

clean:
	$(RM) $(OBJS) $(TARGET_LIB)
