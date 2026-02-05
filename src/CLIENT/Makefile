CC = gcc
CFLAGS = -g -Wall -Wextra -O2
RM = rm -f

RELEASE_DIR ?= ../release
TARGET_NAME := $(notdir $(abspath ./))
TARGET ?= $(RELEASE_DIR)/$(TARGET_NAME)

SRCS = ZT_main.c \
       ZT_ctx.c	 \
	   ZT_evt.c  \
       ZT_hdl.c  \

OBJDIR := $(RELEASE_DIR)
OBJS = $(SRCS:%.c=$(OBJDIR)/%.o)

all: $(TARGET)

$(RELEASE_DIR):
	mkdir -p $(RELEASE_DIR)

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	$(RM) $(OBJS) $(TARGET)

