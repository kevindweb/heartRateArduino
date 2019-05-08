# Ensure that these targets always run
.PHONY: test clean

# DO NOT CHANGE THESE FLAGS
CC=gcc
CFLAGS=-std=c99 -Wall -Wextra -pedantic -Werror

OBJS=final-project.o

all: final-project.o

test: $(OBJS)
	$(CC) $(CFLAGS) -I./ final-project.c histogram.c -o final-project.out -lm
	@echo "Running Final Project"
	./final-project.out

reset: $(OBJS)
	$(CC) $(CFLAGS) -I./ final-project.c histogram.c -o final-project.out -lm
	@echo "Running Final Project"
	echo "" > mmapped.bin
	./final-project.out 1

clean:
	# delete compiled files
	rm -f final-project.out
	rm -f final-project.o 
