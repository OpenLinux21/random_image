# random_image - Makefile
CC      ?= gcc
CFLAGS  += -Wall -Wextra -std=c11 -O2
LDLIBS   = -lm

TARGET  = random_image
SRCS    = main.c random_source.c jpg_writer.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

random_source.o: random_source.c random_source.h
jpg_writer.o: jpg_writer.c jpg_writer.h

clean:
	$(RM) $(TARGET) $(OBJS)
