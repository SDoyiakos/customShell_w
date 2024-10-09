CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -std=gnu18
LOGIN = doyiakos
SUBMITPATH = ~cs537-1/handin/$(LOGIN)/p3/

all: wsh

wsh: wsh.c wsh.h
	$(CC) $< $(CFLAGS) -O2 -o $@

wsh-dbg: wsh.c wsh.h
	$(CC) $< $(CFLAGS) -Og -ggdb -o $@

submit: clean
	rm -r -f ~cs537-1/handin/doyiakos/p3
	cp -r ../../p3 ~cs537-1/handin/doyiakos/p3

clean: 
	rm -f wsh 
	rm -f wsh-dbg
	echo "All files cleaned"
