PROGS = cavescroll cgame difftree fileselect invaders mastermind menu playlist scca snake snowcrash storage sudoku racer

.PHONY: all clean

all clean:
	for PROG in $(PROGS); do \
	  $(MAKE) -C $$PROG $@; \
	done

