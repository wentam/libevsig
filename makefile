# -- config
CC ?= clang
prefix ?= /usr/local/
# -- end config

INCLUDE = -Iinclude/
CFLAGS = -Wall -mavx2 -msse2 -ffast-math -pthread $(INCLUDE) -flto -std=gnu23 -fwrapv -march=x86-64-v3 -fno-strict-aliasing -fzero-call-used-regs=skip -Wno-bitwise-instead-of-logical


rwildcard=$(wildcard $1$2) $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2))

SRCS = $(call rwildcard,src/libevsig/,*.c)
OBJS = $(SRCS:src/libevsig/%.c=build/libevsig/%.o)
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
#dev_fast: IWYU = 1
dev_fast: CFLAGS += -g -Og

.PHONY: dev
dev: default
dev: IWYU = 1
dev: CFLAGS += -g -Og -fsanitize=address

.PHONY: default
default: lib cli

.PHONY: lib
lib: build/libevsig/libevsig.so

.PHONY: cli
cli: build/cli/evsig-cli

.PHONY: clean
clean:
	rm -rf build/

build/libevsig/:
	mkdir -p build/libevsig

build/cli/:
	mkdir -p build/cli

build/libevsig/libevsig.so: build/libevsig/ $(OBJS)
	$(CC) $(CFLAGS) -rdynamic -lm -shared -o $@ $(OBJS)

build/cli/evsig-cli: build/cli/ $(CLI_OBJS) build/libevsig/libevsig.so
	$(CC) $(CFLAGS) -Lbuild/libevsig/ -levsig -rdynamic -o $@ $(CLI_OBJS)

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
	mkdir -p ${DESTDIR}${prefix}/include/libevsig/
	cp build/libevsig/libevsig.so ${DESTDIR}${prefix}/lib/
	cp -r include/libevsig/* ${DESTDIR}${prefix}/include/libevsig/
