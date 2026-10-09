# random_image - Makefile
CC      ?= gcc
CFLAGS  += -Wall -Wextra -std=c11 -O2
LDLIBS   = -lm

TARGET  = random_image
SRCS    = main.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) $(TARGET) $(OBJS)
