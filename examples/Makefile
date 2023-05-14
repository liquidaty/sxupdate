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
BUILD_DIR=$(shell cd ${THIS_MAKEFILE_DIR}/../build/${BUILD_SUBDIR}/${CCBN} && pwd)

WIN=

ifeq (${WIN},)
  ifneq ($(findstring w64,$(CC)),) # win64
    WIN=64
  endif
endif

ifeq (${WIN},)
  WIN=0
endif

ifneq (${WIN},0)
  EXE=.exe
else
  EXE=
endif

TEST_EXE=${BUILD_DIR}/test${EXE}
DUMMY_INSTALLER=${BUILD_DIR}/dummy_installer${EXE}

YAJL_SRC_DIR=${THIS_MAKEFILE_DIR}/../external/yajl
YAJL_SRC1=yajl yajl_alloc yajl_buf yajl_encode yajl_gen yajl_lex yajl_parser yajl_tree yajl_version
YAJL_SRC=$(addprefix ${YAJL_SRC_DIR}/src/,$(addsuffix .c,${YAJL_SRC1}))

ifneq ($(SSL_PREFIX),$(PREFIX))
  INCLUDEDIR+= -I${SSL_PREFIX}/include
  LDFLAGS+=-L${SSL_PREFIX}/lib
endif
INCLUDEDIR+= -I${YAJL_SRC_DIR}/build/yajl-2.1.1/include

LDFLAGS+= -lsxupdate -lyajl_helper ${LDFLAGS_CURL} -L${LIBDIR}
ifneq ($(UNAME_S),Darwin)
  LDFLAGS+=-static -lbcrypt
else
  ifneq ($(WIN),0)
    LDFLAGS+=-static -lbcrypt
  endif
endif

CFLAGS+= ${CFLAGS_CURL}

help:
	@echo "Makefile for use with GNU Make and gcc. Set DEBUG=1 to compile with -g -O0"
	@echo "  make [DEBUG=1] all|test-simple"
	@echo
	@echo "To make with a specified config file:"
	@echo "  make CONFIGFILE=/path/to/config ..."
	@echo

all: ${TEST_EXE} ${DUMMY_INSTALLER}
	@echo "Built $^"

test-simple: ${TEST_EXE} ${DUMMY_INSTALLER} ${BUILD_DIR}/dummy_appcast.json ${BUILD_DIR}/dummy_public_key.pem
ifeq ($(WIN),0)
	echo 'file://${BUILD_DIR}/dummy_appcast.json\nY' | ${TEST_EXE}
else
	echo "Built $^"
endif

${BUILD_DIR}/dummy_private_key.pem:
	openssl genpkey -algorithm RSA -out $@

${BUILD_DIR}/dummy_public_key.pem: ${BUILD_DIR}/dummy_private_key.pem
	openssl rsa -pubout -in $< -out $@

${BUILD_DIR}/dummy_appcast.json: simple/appcast.json.in ${BUILD_DIR}/dummy_signature.txt
	@mkdir -p `dirname "$@"`
	cat $< | sed 's#THIS_DIR#${BUILD_DIR}#' | sed "s@SIGNATURE@`cat ${BUILD_DIR}/dummy_signature.txt`@" > $@.tmp
	mv $@.tmp $@

${BUILD_DIR}/dummy_signature.txt: ${BUILD_DIR}/dummy_signature.sig
	openssl base64 -A -in $< > $@

clean:
	rm -rf ${TEST_EXE} ${BUILD_DIR}

${DUMMY_INSTALLER}: simple/dummy_installer.c
	@mkdir -p `dirname "$@"`
	${CC} ${CFLAGS} -I${INCLUDEDIR} $< ${YAJL_SRC} -o $@

${BUILD_DIR}/dummy_signature.sig: ${DUMMY_INSTALLER} ${BUILD_DIR}/dummy_private_key.pem
	openssl dgst -sha256 -sign ${BUILD_DIR}/dummy_private_key.pem -out $@ $<

${TEST_EXE}: test.c
	@mkdir -p `dirname "$@"`
	${CC} ${CFLAGS} -I${INCLUDEDIR} $< ${YAJL_SRC} -o $@  ${LDFLAGS}

# to verify: openssl dgst -sha256 -verify public_key.pem -signature win/dummy_signature.bin win/dummy_installer.exe
