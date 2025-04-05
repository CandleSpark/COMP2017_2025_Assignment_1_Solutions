CC = gcc
CFLAGS = -Wall -Wextra -Werror -fPIC -g
LDFLAGS = -lm

all: sound_editor

sound_editor: main.o sound_seg.o
	$(CC) -o sound_editor main.o sound_seg.o $(LDFLAGS)

main.o: main.c sound_seg.h
	$(CC) $(CFLAGS) -c main.c

sound_seg.o: sound_seg.c sound_seg.h
	$(CC) $(CFLAGS) -c sound_seg.c

.PHONY: clean
clean:
	rm -f sound_editor *.o 