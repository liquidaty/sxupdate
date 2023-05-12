THIS_MAKEFILE_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

CONFIGFILE ?= ../config.mk
$(info Using config file ${CONFIGFILE})
include ${CONFIGFILE}

CONFIGFILEPATH=$(shell ls ${CONFIGFILE} >/dev/null 2>/dev/null && realpath ${CONFIGFILE})
ifeq ($(CONFIGFILEPATH),)
  $(error Config file ${CONFIGFILE} not found)
endif

BUILD_DIR = $(shell cd ${THIS_MAKEFILE_DIR}/.. && pwd)/build/test

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

LIBS=-lsxupdate -lcurl -lyajl_helper

YAJL_SRC_DIR=${THIS_MAKEFILE_DIR}/../external/yajl
YAJL_SRC1=yajl yajl_alloc yajl_buf yajl_encode yajl_gen yajl_lex yajl_parser yajl_tree yajl_version
YAJL_SRC=$(addprefix ${YAJL_SRC_DIR}/src/,$(addsuffix .c,${YAJL_SRC1}))

INCLUDEDIR+= -I${YAJL_SRC_DIR}/build/yajl-2.1.1/include

CFLAGS+= -g -O0

all: ${TEST_EXE}

test-simple: ${TEST_EXE} ${DUMMY_INSTALLER} ${BUILD_DIR}/dummy_appcast.json
	echo 'file://${BUILD_DIR}/dummy_appcast.json\nY' | ${TEST_EXE}

${BUILD_DIR}/dummy_appcast.json: simple/appcast.json.in
	@mkdir -p `dirname "$@"`
	cat $< | sed 's#THIS_DIR#${BUILD_DIR}#' > $@.tmp
	mv $@.tmp $@

clean:
	rm -rf ${TEST_EXE} ${BUILD_DIR}

${DUMMY_INSTALLER}: simple/dummy_installer.c
	@mkdir -p `dirname "$@"`
	${CC} ${CFLAGS} -I${INCLUDEDIR} -L${LIBDIR} ${LIBS} $< ${YAJL_SRC} -o $@

${TEST_EXE}: test.c
	@mkdir -p `dirname "$@"`
	${CC} ${CFLAGS} -I${INCLUDEDIR} -L${LIBDIR} ${LIBS} $< ${YAJL_SRC} -o $@
