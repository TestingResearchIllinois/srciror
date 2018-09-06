all: max

CC := clang

max: max.c
	${CC} -O0 -o $@ $(shell pwd)/$<

clean:
	rm -f max

test: max
	./max
