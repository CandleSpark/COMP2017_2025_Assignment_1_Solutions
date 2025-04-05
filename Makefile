CC = gcc
CFLAGS = -Wall -Wextra -Werror -Wvla -fPIC -g
LDFLAGS = -lm

all: sound_editor

# Rule required by assignment spec
sound_seg.o: sound_seg.c sound_seg.h
	$(CC) $(CFLAGS) -c sound_seg.c -o sound_seg.o

# Make the executable depend on the required object file
sound_editor: main.o sound_seg.o
	$(CC) $(CFLAGS) main.o sound_seg.o -o sound_editor $(LDFLAGS)

main.o: main.c sound_seg.h
	$(CC) $(CFLAGS) -c main.c

.PHONY: clean
clean:
	rm -f sound_editor *.o 