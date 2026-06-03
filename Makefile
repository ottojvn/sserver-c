##
# sserver-c
#
# @file
# @version 0.1

.DEFAULT_GOAL := dev

CC = clang
WARNINGS = -Wall -Wextra -Wpedantic \
-Wshadow \
-Wconversion -Wsign-conversion \
-Wformat=2 \
-Wnull-dereference \
-Wcast-qual -Wcast-align \
-Wvla \
-Wundef \
-Wmissing-prototypes -Wmissing-declarations \
-Wimplicit-fallthrough \
-Wstrict-overflow=2 \
-Wwrite-strings
STD=-std=c23
FILE=server.c

.PHONY: dev prod clean

dev: CFLAGS = $(WARNINGS) -O0 -g3 \
	-fno-omit-frame-pointer \
	-fsanitize=address,undefined,leak
dev: LDFLAGS = -fsanitize=address,undefined,leak -pthread
dev: build/dev/server


prod: CFLAGS = $(WARNINGS) -Werror \
  -O2 -g \
  -fstack-protector-strong \
  -fstack-clash-protection \
  -fcf-protection=full \
  -D_FORTIFY_SOURCE=3 \
  -fPIE
prod: LDFLAGS = -pie \
  -Wl,-z,relro -Wl,-z,now \
  -pthread
prod: build/prod/server

build/dev/server: $(FILE)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(LDFLAGS) $(STD) -o $@ $(FILE)

build/prod/server: $(FILE)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(LDFLAGS) $(STD) -o $@ $(FILE)

clean:
	rm -rf build/

# end
