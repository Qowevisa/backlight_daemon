CC = gcc
CFLAGS = -g -O2 -Wall -Wextra -Werror -pedantic
project_name = tt
client = client

build: 
	$(CC) -o $(project_name) $(project_name).c $(CFLAGS)
	$(CC) -o $(client) $(client).c $(CFLAGS)

clean: 
	-rm $(project_name) $(client) 2> /dev/null

.PHONY: build clean
