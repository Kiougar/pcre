first: all

%.o: %.c
	cc -c -Wall -Wextra -g -o $@ $^

main: matcher.o classifier.o main.o
	cc -g -o $@ $^ -lpcre

all: main

clean:
	rm -f *.o
	rm -f main
