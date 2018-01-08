CFLAGS=-Wall

all: cavescroll

cavescroll.o: cavescroll.c
	gcc -c cavescroll.c ${CFLAGS}

cavescroll: cavescroll.o
	gcc -o cavescroll cavescroll.o ${CFLAGS} -lncurses

.PHONY: clean
clean:
	rm -f *.o cavescroll 

