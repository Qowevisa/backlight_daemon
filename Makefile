CC = gcc
CFLAGS = -g -O2 -Wall -Wextra -Werror -pedantic
daemon = lghtd
server = server
client = client
time = time

build:
	$(CC) -o $(daemon) $(daemon).c $(CFLAGS)
	$(CC) -o $(server) $(server).c $(CFLAGS)
	$(CC) -o $(client) $(client).c $(CFLAGS)
	$(CC) -o $(time) $(time).c $(CFLAGS)

clean: 
	-rm $(project_name) $(client) 2> /dev/null

tarball:
	tar -czvf daemon.tar.gz $(daemon).c $(server).c $(client).c $(time).c Makefile

.PHONY: build clean
