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

INCLUDE_DIR+= -I${THIS_MAKEFILE_DIR}/external/yajl/build/yajl-2.1.1/include -I${PREFIX}/include

ifneq ($(SSL_PREFIX),$(PREFIX))
  INCLUDE_DIR+= -I${SSL_PREFIX}/include
endif

OBJ_SRC=verify api file fork_and_exit version parse

PKGCONFIGLIBS=
ifeq ($(USE_BUNDLED_YAJL),1)
  OBJ_SRC+=external/yajl_helper/yajl_helper external/yajl/src/yajl_parser external/yajl/src/yajl_alloc external/yajl/src/yajl_lex external/yajl/src/yajl_gen external/yajl/src/yajl_tree external/yajl/src/yajl_version external/yajl/src/yajl_buf external/yajl/src/yajl external/yajl/src/yajl_encode
else
  PKGCONFIGLIBS+=-lyajl
endif

PKGCONFIGLIBS+=${LDFLAGS_CURL}

CFLAGS+= ${CFLAGS_CURL} ${INCLUDE_DIR} -Wall -Wextra -Wno-missing-field-initializers -Wunused

OBJS=$(addprefix ${BUILD_DIR}/, $(addsuffix .o, ${OBJ_SRC}))


LIBDIR=${PREFIX}/lib
PKGCONFIGDIR=${LIBDIR}/pkgconfig

INSTALLED_LIB=${LIBDIR}/libsxupdate.a
INSTALLED_HEADERS=$(addprefix ${PREFIX}/include/sxupdate/, $(addsuffix .h, api))

INSTALLED_PKGCONFIG=${PKGCONFIGDIR}/sxupdate.pc

help:
	@echo "Makefile for use with GNU Make and gcc. Set DEBUG=1 to compile with -g -O0"
	@echo "  make [DEBUG=1] build|install|uninstall|clean"
	@echo
	@echo "To make with a specified config file:"
	@echo "  make CONFIGFILE=/path/to/config ..."
	@echo


build: ${OBJS} ${BUILD_DIR}/pkgconfig/sxupdate.pc


install: ${INSTALLED_LIB} ${INSTALLED_HEADERS} ${INSTALLED_PKGCONFIG}


${INSTALLED_PKGCONFIG}: ${BUILD_DIR}/pkgconfig/sxupdate.pc
	install -m 644 $< "`dirname "$@"`"



VERSION=$(shell cat version.txt)
${BUILD_DIR}/pkgconfig/sxupdate.pc: pkgconfig/sxupdate.pc.in version.txt
	@mkdir -p `dirname "$@"`
	cat $< | sed 's#PREFIX#${PREFIX}#' \
	| sed 's#VERSION#${VERSION}#' \
	| sed 's#PKGCONFIGLIBS#${PKGCONFIGLIBS}#' \
	> $@

${INSTALLED_HEADERS}: ${PREFIX}/include/sxupdate/%.h : ../include/%.h
	@mkdir -p `dirname "$@"`
	install -m 644 $< "`dirname "$@"`"
	@echo "Installed $@"

${INSTALLED_LIB}: ${BUILD_DIR}/lib/libsxupdate.a
	@mkdir -p `dirname "$@"`
	install -m 644 $< "`dirname "$@"`"
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
