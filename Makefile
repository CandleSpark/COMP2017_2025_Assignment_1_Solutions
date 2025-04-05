CC = gcc
CFLAGS = -Wall -Wextra -Werror -fPIC -g
LDFLAGS = -lm

sound_seg.o: sound_seg.c sound_seg.h
	$(CC) $(CFLAGS) -c sound_seg.c $(LDFLAGS)

.PHONY: clean
clean:
	rm -f sound_seg.o 