CROSS_COMPILE ?=

CC      := $(CROSS_COMPILE)gcc
AR      := $(CROSS_COMPILE)ar
CFLAGS = -fPIC -Wall -Wint-conversion
LDFLAGS = -shared
TARGET_LIB = libdrp.so

SRCS = drp-lib.c
OBJS = $(SRCS:.c=.o)

.PHONY: all
all: ${TARGET_LIB}

$(TARGET_LIB): $(OBJS)
	$(CC) ${LDFLAGS} -o $@ $^
	$(AR) rcs libdrp.a $(OBJS)

.PHONY: install	
install:
	cp libdrp.so $(BUILDROOT_DIR)/output/target/usr/lib
	@echo ''
	@echo 'Please run Buildroot again to repackage the rootfs image.'

.PHONY: clean
clean:
	rm -f ${TARGET_LIB} ${OBJS} *.a
	rm -f drplib-config-api.h
	rm -f drplib-config.bin
