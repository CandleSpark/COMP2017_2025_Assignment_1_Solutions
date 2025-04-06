CC = gcc
CFLAGS = -Wall -Wextra -g -fsanitize=address
LDFLAGS = -fsanitize=address -lm

SRCS = sound_seg.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean editor

all: sound_editor

sound_editor: main.o sound_seg.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

main.o: main.c sound_seg.h
	$(CC) $(CFLAGS) -c $< -o $@

editor: sound_editor
	./sound_editor

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f sound_editor *.o 