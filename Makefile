CC = gcc
CFLAGS = -g -O2 -Wall -Wextra -Werror -pedantic
daemon = lghtd
server = server
client = client
time = time
tarball_name = daemon.tar.gz
tarball_small_name = daemon.small.tar.gz

build:
	$(CC) -o $(daemon) $(daemon).c $(CFLAGS)
	$(CC) -o $(server) $(server).c $(CFLAGS)
	$(CC) -o $(client) $(client).c $(CFLAGS)
	$(CC) -o $(time) $(time).c $(CFLAGS)

clean: 
	-rm $(tarball_name) $(tarball_small_name) $(daemon) $(server) $(client) $(time) 2> /dev/null

tarball:
	tar -czvf $(tarball_name) $(daemon).c $(server).c $(client).c $(time).c Makefile

sml_tarball:
	tar -czvf $(tarball_small_name) $(daemon).c $(client).c

.PHONY: build clean
