CC := gcc
CFLAGS := -Wall -Wextra
LDFLAGS := -lconfuse -lbsd
REL_FLAGS := -O2 -D DISABLE_DEBUG
DBG_FLAGS := -ggdb -O0

JWMS_LDFLAGS := -lX11
JWMS_REL_FLAGS := -O2

ARCH := $(shell uname -m)

ifeq ($(ARCH), i686)
    ARCH := x86
else ifeq ($(ARCH), i386)
    ARCH := x86
endif

BIN := jwm-helper
BIN2 := jwms
DESKTOP := jwms.desktop
CONF := jwms.conf
VERSION ?= 0.2
TARBALL_NAME := jwms-$(VERSION)-linux-$(ARCH).tar.gz
SYS_CONF_DIR := $(DESTDIR)/etc/jwms
BIN_DIR := $(DESTDIR)/usr/local/bin
DESKTOP_DIR := $(DESTDIR)/usr/share/xsessions

# Posix compatiable version of $(wildcard)
#SRCS := $(shell echo src/*.c)
#OBJS := $(SRCS:src/%.c=%.o)
JWMS_SRC := src/jwms.c
SRCS := $(filter-out $(JWMS_SRC), $(shell echo src/*.c))
OBJS := $(filter-out jwms.o, $(SRCS:src/%.c=%.o))

JWMS_OBJ := $(JWMS_SRC:src/%.c=%.o)

BUILD_DIR := build
REL_DIR := $(BUILD_DIR)/release
DBG_DIR := $(BUILD_DIR)/debug

DBG_OBJS := $(addprefix $(DBG_DIR)/, $(OBJS))
REL_OBJS := $(addprefix $(REL_DIR)/, $(OBJS))

JWMS_DBG_OBJ := $(addprefix $(DBG_DIR)/, $(JWMS_OBJ))
JWMS_REL_OBJ := $(addprefix $(REL_DIR)/, $(JWMS_OBJ))

REL_BIN := $(REL_DIR)/$(BIN)
DBG_BIN := $(DBG_DIR)/$(BIN)

JWMS_REL_BIN := $(REL_DIR)/$(BIN2)
JWMS_DBG_BIN := $(DBG_DIR)/$(BIN2)

.PHONY: all clean release debug run install uninstall tarball

all: release

release: $(REL_BIN) $(JWMS_REL_BIN)
	@cp $(REL_BIN) $(BIN)
	@cp $(JWMS_REL_BIN) $(BIN2)

$(REL_BIN): $(REL_OBJS)
	$(CC) $(REL_FLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(JWMS_REL_BIN): $(JWMS_REL_OBJ)
	$(CC) $(JWMS_REL_FLAGS) $(CFLAGS) -o $@ $^ $(JWMS_LDFLAGS)

$(REL_DIR)/%.o: src/%.c
	@mkdir -p $(REL_DIR)
	@if [ "$*" = "jwms" ]; then \
		$(CC) $(JWMS_REL_FLAGS) $(CFLAGS) -c -o $@ $<; \
	else \
		$(CC) $(REL_FLAGS) $(CFLAGS) -c -o $@ $<; \
	fi

debug: $(DBG_BIN) $(JWMS_DBG_BIN)
	@cp $(DBG_BIN) $(BIN)
	@cp $(JWMS_DBG_BIN) $(BIN2)

$(DBG_BIN): $(DBG_OBJS)
	$(CC) $(DBG_FLAGS) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(JWMS_DBG_BIN): $(JWMS_DBG_OBJ)
	$(CC) $(DBG_FLAGS) $(CFLAGS) -o $@ $^ $(JWMS_LDFLAGS)

$(DBG_DIR)/%.o: src/%.c
	@mkdir -p $(DBG_DIR)
	$(CC) $(DBG_FLAGS) $(CFLAGS) -c -o $@ $<

run:
	./jwm-helper -a

clean:
	@rm -rf $(BUILD_DIR)
	@if [ -f "$(BIN)" ]; then rm $(BIN); fi
	@if [ -f "$(BIN2)" ]; then rm $(BIN2); fi
	@if [ -f "$(TARBALL_NAME)" ]; then rm $(TARBALL_NAME); fi

tarball:
	@if [ ! -f "$(BIN)" ] && [ ! -f "$(BIN2)" ]; then \
		echo "Please run make before creating a tarball."; \
	else \
		echo "Creating tarball $(TARBALL_NAME)..."; \
		strip $(BIN); \
		strip $(BIN2); \
		tar -czf $(TARBALL_NAME) $(BIN) $(BIN2) jwms.desktop jwms.conf install.sh; \
	fi

install:
	@if [ ! -f "$(BIN)" ] && [ ! -f "$(BIN2)" ]; then \
		echo "Please run make before installing."; \
	else \
		echo "Installing $(BIN) to $(BIN_DIR)"; \
		install -d $(BIN_DIR); \
		install $(BIN) $(BIN_DIR)/$(BIN); \
		echo "Installing $(BIN2) to $(BIN_DIR)"; \
		install $(BIN2) $(BIN_DIR)/$(BIN2); \
		install -d $(DESKTOP_DIR); \
		echo "Installing $(DESKTOP) to $(DESKTOP_DIR)"; \
		install -m 644 $(DESKTOP) $(DESKTOP_DIR)/$(DESKTOP); \
		install -d -m 0755 $(SYS_CONF_DIR); \
		echo "Installing $(CONF) to $(SYS_CONF_DIR)"; \
		install -m 644 $(CONF) $(SYS_CONF_DIR)/$(CONF); \
	fi

uninstall:
	rm -f $(BIN_DIR)/$(BIN)
	rm -f $(BIN_DIR)/$(BIN2)
	rm -f $(DESKTOP_DIR)/$(DESKTOP)
	rm -f $(SYS_CONF_DIR)/$(CONF)
