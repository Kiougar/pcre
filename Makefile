first: all

%.o: %.c
	cc -c -Wall -Wextra -g -o $@ $^

gonzo: matcher.o gonzo.o
	cc -g -o $@ $^ -lpcre

all: gonzo

clean:
	rm -f *.o
	rm -f gonzo
