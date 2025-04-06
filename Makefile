CC = gcc
CFLAGS = -Wall -Wextra -g -fsanitize=address
LDFLAGS = -fsanitize=address

SRCS = sound_seg.c
OBJS = $(SRCS:.c=.o)
TEST_SRCS = tests/test_sound_seg.c
TEST_OBJS = $(TEST_SRCS:.c=.o)

.PHONY: all clean test

all: libsoundseg.a test_sound_seg

libsoundseg.a: $(OBJS)
	$(CC) -c -o $@ $^
	ar rcs $@ $^

test_sound_seg: $(TEST_OBJS) libsoundseg.a
	$(CC) $(LDFLAGS) -o $@ $^

test: test_sound_seg
	./test_sound_seg

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TEST_OBJS) libsoundseg.a test_sound_seg 