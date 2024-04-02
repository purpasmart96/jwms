CC:=gcc
CFLAGS:=-Wall -Wextra
LDFLAGS:=-lconfuse -lbsd
REL_FLAGS:=-O2
DBG_FLAGS:=-g -Og

# Posix compatiable version of $(wildcard)
SRCS:=$(shell echo src/*.c)
#SRCS=src/ini.c src/hashing.c src/jwms.c
#SRCS:=$(wildcard src/*.c)
OBJS:=$(SRCS:src/%.c=%.o)

BUILD_DIR:=build
REL_DIR:=$(BUILD_DIR)/release
DBG_DIR:=$(BUILD_DIR)/debug

DBG_OBJS:=$(addprefix $(DBG_DIR)/, $(OBJS))
REL_OBJS:=$(addprefix $(REL_DIR)/, $(OBJS))

BIN:=jwms
REL_BIN:=$(REL_DIR)/$(BIN)
DBG_BIN:=$(DBG_DIR)/$(BIN)

.PHONY: all clean release debug

all: release

release: $(REL_BIN)
	@cp $< $(BIN)

$(REL_BIN): $(REL_OBJS)
	$(CC) $(REL_FLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(REL_DIR)/%.o: src/%.c
	@mkdir -p $(REL_DIR)
	$(CC) $(REL_FLAGS) $(CFLAGS) -c -o $@ $<

debug: $(DBG_BIN)
	cp $< $(BIN)

$(DBG_BIN): $(DBG_OBJS)
	$(CC) $(DBG_FLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(DBG_DIR)/%.o: src/%.c
	@mkdir -p $(DBG_DIR)
	$(CC) $(DBG_FLAGS) $(CFLAGS) -c -o $@ $<

clean:
	@rm -rf $(BUILD_DIR)
	@if [ -f "$(BIN)" ]; then rm $(BIN); fi
