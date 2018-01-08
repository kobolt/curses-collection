FLAGS=-Wall

all: playlist

play.o: play.c
	gcc -c play.c -o play.o ${FLAGS}

ui.o: ui.c
	gcc -c ui.c -o ui.o ${FLAGS}

list.o: list.c
	gcc -c list.c -o list.o ${FLAGS}

playlist: ui.o list.o play.o
	gcc -lncurses ui.o list.o play.o -o playlist ${FLAGS}

.PHONY: clean
clean:
	rm -f *.o playlist

