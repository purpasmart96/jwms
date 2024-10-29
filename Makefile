CC := gcc
CFLAGS := -Wall -Wextra
LDFLAGS := -lconfuse -lbsd
REL_FLAGS := -O2 -D DISABLE_DEBUG
DBG_FLAGS := -ggdb -O0

BIN := jwms
VERSION ?= 0.1
TARBALL_NAME := jwms-$(VERSION).tar.gz
SYS_CONF := /etc/jwms
DEST_DIR := /usr/local/bin

# Posix compatiable version of $(wildcard)
SRCS := $(shell echo src/*.c)
OBJS := $(SRCS:src/%.c=%.o)

BUILD_DIR := build
REL_DIR := $(BUILD_DIR)/release
DBG_DIR := $(BUILD_DIR)/debug

DBG_OBJS := $(addprefix $(DBG_DIR)/, $(OBJS))
REL_OBJS := $(addprefix $(REL_DIR)/, $(OBJS))

REL_BIN := $(REL_DIR)/$(BIN)
DBG_BIN := $(DBG_DIR)/$(BIN)

.PHONY: all clean release debug run install uninstall

all: release

release: $(REL_BIN)
	@cp $< $(BIN)

$(REL_BIN): $(REL_OBJS)
	$(CC) $(REL_FLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(REL_DIR)/%.o: src/%.c
	@mkdir -p $(REL_DIR)
	$(CC) $(REL_FLAGS) $(CFLAGS) -c -o $@ $<

debug: $(DBG_BIN)
	@cp $< $(BIN)

$(DBG_BIN): $(DBG_OBJS)
	$(CC) $(DBG_FLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(DBG_DIR)/%.o: src/%.c
	@mkdir -p $(DBG_DIR)
	$(CC) $(DBG_FLAGS) $(CFLAGS) -c -o $@ $<

run:
	./jwms -a

clean:
	@rm -rf $(BUILD_DIR)
	@if [ -f "$(BIN)" ]; then rm $(BIN); fi
	@if [ -f "$(TARBALL_NAME)" ]; then rm $(TARBALL_NAME); fi

tarball: release
	@echo "Creating tarball $(TARBALL_NAME)..."
	tar -czf $(TARBALL_NAME) $(BIN) jwms.conf install.sh

install: release
	install -d $(DEST_DIR)
	install $(BIN) $(DEST_DIR)/$(BIN)
	install -d -m 0755 $(SYS_CONF)
	install -m 644 jwms.conf $(SYS_CONF)/jwms.conf

uninstall:
	rm -f $(DEST_DIR)/$(BIN)
	rm -f $(SYS_CONF)/jwms.conf
