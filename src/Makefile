THIS_MAKEFILE_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

CONFIGFILE ?= ../config.mk
$(info Using config file ${CONFIGFILE})
include ${CONFIGFILE}

CONFIGFILEPATH=$(shell ls ${CONFIGFILE} >/dev/null 2>/dev/null && realpath ${CONFIGFILE})
ifeq ($(CONFIGFILEPATH),)
  $(error Config file ${CONFIGFILE} not found)
endif

UNAME_S := $(shell uname -s)
DEBUG=0
ifeq ($(DEBUG),1)
  CFLAGS+=-g -O0
  DBG_SUBDIR=dbg
else
  CFLAGS+=-O3 -DNDEBUG
  DBG_SUBDIR=rel
endif

ifeq ($(WIN),0)
  BUILD_SUBDIR=${UNAME_S}/${DBG_SUBDIR}
else
  BUILD_SUBDIR=win/${DBG_SUBDIR}
endif
CCBN=$(shell basename ${CC})
BUILD_DIR=${THIS_MAKEFILE_DIR}/../build/${BUILD_SUBDIR}/${CCBN}

INCLUDE_DIR+= -I${THIS_MAKEFILE_DIR}/../external/yajl/build/yajl-2.1.1/include -I${PREFIX}/include

ifneq ($(SSL_PREFIX),$(PREFIX))
  INCLUDE_DIR+= -I${SSL_PREFIX}/include
endif

OBJ_SRC=verify api file fork_and_exit version parse

CFLAGS+= ${CFLAGS_CURL} ${INCLUDE_DIR} -Wall -Wextra -Wno-missing-field-initializers -Wunused

OBJS=$(addprefix ${BUILD_DIR}/, $(addsuffix .o, ${OBJ_SRC}))

INSTALLED_LIB=${PREFIX}/lib/libsxupdate.a
INSTALLED_HEADERS=$(addprefix ${PREFIX}/include/sxupdate/, $(addsuffix .h, api))

help:
	@echo "Makefile for use with GNU Make and gcc. Set DEBUG=1 to compile with -g -O0"
	@echo "  make [DEBUG=1] build|install|uninstall|clean"
	@echo
	@echo "To make with a specified config file:"
	@echo "  make CONFIGFILE=/path/to/config ..."
	@echo


build: ${OBJS}


install: ${INSTALLED_LIB} ${INSTALLED_HEADERS}


${INSTALLED_HEADERS}: ${PREFIX}/include/sxupdate/%.h : ../include/%.h
	@mkdir -p `dirname "$@"`
	@cp -p $< $@
	@echo "Installed $@"

${INSTALLED_LIB}: ${BUILD_DIR}/lib/libsxupdate.a
	@mkdir -p `dirname "$@"`
	@cp -p $< $@
	@echo "Installed $@"

${BUILD_DIR}/lib/libsxupdate.a: ${OBJS}
	@mkdir -p `dirname "$@"`
	@$(AR) rcv $@ $^
	@$(RANLIB) $@
	@$(AR) -t $@ # check it is there
	@echo Built $@

${OBJS}: ${BUILD_DIR}/%.o : %.c
	@mkdir -p `dirname "$@"`
	${CC} ${CFLAGS} -c $< -o $@

uninstall:
	rm -rf ${INSTALLED_LIB} ${INSTALLED_HEADERS}

clean:
	rm -rf ${OBJS} ${BUILD_DIR}

.PHONY: help build install uninstall clean
