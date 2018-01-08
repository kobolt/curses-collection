
all: invaders

invaders: loop.o iface.o game.o
	gcc -o invaders loop.o iface.o game.o -lncurses -Wall

loop.o: loop.c game.h iface.h
	gcc -o loop.o -c loop.c -Wall

iface.o: iface.c iface.h game.h
	gcc -o iface.o -c iface.c -Wall

game.o: game.c game.h
	gcc -o game.o -c game.c -Wall

.PHONY: clean
clean:
	rm -f *.o
	rm -f invaders

