TARGETS = oss user_proc
CC = gcc
CFLAGS  = -g -Wall -std=c99

all: clean $(TARGETS)

$(TARGETS):
	$(CC) $(CFLAGS) $@.c -o $@

clean: 
	rm -f $(TARGETS)
