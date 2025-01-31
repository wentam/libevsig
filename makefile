# -- config
CC ?= clang
prefix ?= /usr/local/
# -- end config

$(info $(.VARIABLES))

INCLUDE = -Iinclude/ -I.
CFLAGS = -Wall -pthread $(INCLUDE) # -Wconversion

rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

SRCS = $(call rwildcard,src/lib/,*.c)
OBJS = $(SRCS:src/lib/%.c=build/lib/%.o)
DEPS = $(OBJS:%.o=%.d)

CLI_SRCS = $(call rwildcard,src/cli/,*.c)
CLI_OBJS = $(CLI_SRCS:src/cli/%.c=build/cli/%.o)
CLI_DEPS = $(CLI_OBJS:%.o=%.d)

IWYU = 0

.PHONY: release
release: default
release: CFLAGS += -O3

.PHONY: dev_fast
dev_fast: default
dev_fast: IWYU = 1
dev_fast: CFLAGS += -g -Og -Werror

.PHONY: dev
dev: default
dev: IWYU = 1
dev: CFLAGS += -g -Og -Werror -fsanitize=address

.PHONY: default
default: lib cli

.PHONY: lib
lib: build/lib/libevsig.so

.PHONY: cli
cli: build/cli/evsig-cli

.PHONY: clean
clean:
	rm -rf build/

build/lib/:
	mkdir -p build/lib

build/cli/:
	mkdir -p build/cli

build/lib/libevsig.so: build/lib/ $(OBJS)
	$(CC) $(CFLAGS) -lm -shared -o $@ $(OBJS)

build/cli/evsig-cli: build/cli/ $(CLI_OBJS) build/lib/libevsig.so
	$(CC) $(CFLAGS) -Lbuild/lib/ -levsig -o $@ $(CLI_OBJS)

-include $(DEPS)
-include $(CLI_DEPS)

build/%.o: src/%.c
	$(CC) $(CFLAGS) -fPIC -MMD -c $< -o $@
	@if [ $(IWYU) -eq 1 ]; then \
		iout=$$(include-what-you-use -Xiwyu --error=1 -Xiwyu --no_comments $(CFLAGS) $< 2>&1); \
		if [ $$? -ne 0 ]; then echo "$$iout"; exit 1; fi; \
	fi

.PHONY: install
install:
	mkdir -p ${DESTDIR}${prefix}/lib/
	cp build/lib/libevsig.so ${DESTDIR}${prefix}/lib/
