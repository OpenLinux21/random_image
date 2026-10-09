# random_image - Makefile
CC      ?= gcc
CFLAGS  += -Wall -Wextra -std=c11 -O2
LDLIBS   = -lm

TARGET  = random_image
SRCS    = main.c random_source.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

random_source.o: random_source.c random_source.h

clean:
	$(RM) $(TARGET) $(OBJS)
