# Makefile variables
CC = gcc
CFLAGS = -Wall -Wextra
RM = rm -f
TARGET_LIB = libmsocket.a
AR = ar
MAIN = mysocket.c

OBJS = $(MAIN:.c=.o)
HEADERS = $(MAIN:.c=.h)

# Makefile targets
all: $(TARGET_LIB)
	gcc -g -o server test_server.c -L. -lmsocket
	gcc -g -o client test_client.c -L. -lmsocket

$(TARGET_LIB): $(OBJS)
	$(AR) rcs $@ $^

$(OBJS): $(HEADERS) $(MAIN)
	$(CC) $(CFLAGS) -c $(MAIN)

clean:
	$(RM) $(OBJS) $(TARGET_LIB)
