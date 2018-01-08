CFLAGS=-Wall

all: difftree

node.o: node.c
	gcc -c node.c ${CFLAGS}

tree.o: tree.c
	gcc -c tree.c ${CFLAGS}

navi.o: navi.c
	gcc -c navi.c ${CFLAGS}

main.o: main.c
	gcc -c main.c ${CFLAGS}

difftree: node.o tree.o navi.o main.o
	gcc -o difftree node.o tree.o navi.o main.o ${CFLAGS} -lncurses

.PHONY: clean
clean:
	rm -f *.o difftree 

