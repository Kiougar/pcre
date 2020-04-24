first: all

%.o: %.c
	cc -c -g -o $@ $^

matcher.o: matcher.c

gonzo: matcher.o gonzo.o
	cc -g -o $@ $^ -lpcre

all: gonzo

clean:
	rm -f *.o
	rm -f gonzo
