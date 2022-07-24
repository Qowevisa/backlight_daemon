CC = gcc
CFLAGS = -g -O2 -Wall -Wextra -Werror -pedantic
daemon = lghtd
server = server
client = client
time = time

test:
	$(CC) -o $(time) $(time).c $(CFLAGS)

build:
	$(CC) -o $(daemon) $(daemon).c $(CFLAGS)
	$(CC) -o $(server) $(server).c $(CFLAGS)
	$(CC) -o $(client) $(client).c $(CFLAGS)

clean: 
	-rm $(project_name) $(client) 2> /dev/null

.PHONY: build clean
