CC = gcc
CFLAGS = -g -Wall -Wextra -O2
RM = rm -f

# Makefile이 있는 디렉터리 기준 (다른 경로에서 make 해도 동일 동작)
MKFDIR := $(dir $(abspath $(firstword $(MAKEFILE_LIST))))
RELEASE_DIR ?= release
TARGET_NAME := $(notdir $(patsubst %/,%,$(MKFDIR)))
RB_RELEASE_DIR := $(MKFDIR)../release

SRCS = ZT_main.c \
       ZT_ctx.c  \
       ZT_ctrl.c \
       ZT_evt.c  \
       ZT_hdl.c  \
       ZT_sock.c \

OBJDIR := $(RELEASE_DIR)
OBJS = $(SRCS:%.c=$(OBJDIR)/%.o)

all: $(RELEASE_DIR)/$(TARGET_NAME)

# make rb → release/ 에 빌드 후 바이너리만 상위 release/ 로 mv
rb: $(RELEASE_DIR)/$(TARGET_NAME)
	mkdir -p $(RB_RELEASE_DIR)
	mv $(RELEASE_DIR)/$(TARGET_NAME) $(RB_RELEASE_DIR)/$(TARGET_NAME)

$(OBJDIR):
	mkdir -p $@

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<

$(RELEASE_DIR)/$(TARGET_NAME): $(OBJS) | $(OBJDIR)
	$(CC) $(CFLAGS) -o $@ $^ -lpthread

clean:
	$(RM) $(OBJS) $(RELEASE_DIR)/$(TARGET_NAME)

rb_clean:
	$(RM) $(RB_RELEASE_DIR)/$(TARGET_NAME)

