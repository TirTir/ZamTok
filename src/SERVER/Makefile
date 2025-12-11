CC = gcc
CFLAGS = -g -Wall -Wextra -O2
RM = rm -f

RELEASE_DIR = ../release
TARGET = $(RELEASE_DIR)/main

SRCS = ZT_main.c \
       ZT_ctx.c	 \
	   ZT_evt.c  \
       ZT_hdl.c  \
	   ZT_queue.c

OBJS = $(SRCS:.c=.o)

all: $(RELEASE_DIR) $(TARGET)

$(RELEASE_DIR):
	mkdir -p $(RELEASE_DIR)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	$(RM) $(OBJS) $(TARGET)

