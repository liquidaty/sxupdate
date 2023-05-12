THIS_MAKEFILE_DIR:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

CONFIGFILE ?= ../config.mk
$(info Using config file ${CONFIGFILE})
include ${CONFIGFILE}

CONFIGFILEPATH=$(shell ls ${CONFIGFILE} >/dev/null 2>/dev/null && realpath ${CONFIGFILE})
ifeq ($(CONFIGFILEPATH),)
  $(error Config file ${CONFIGFILE} not found)
endif

BUILD_DIR = $(shell cd ${THIS_MAKEFILE_DIR}/.. && pwd)/build

INCLUDE_DIR+= -I${THIS_MAKEFILE_DIR}/../external/yajl_helper -I${THIS_MAKEFILE_DIR}/../external/yajl/build/yajl-2.1.1/include

OBJ_SRC=parse api file fork_and_exit version

CFLAGS+= ${INCLUDE_DIR} -Wall -Wextra -Wno-missing-field-initializers -Wunused -g -O0

OBJS=$(addprefix ${BUILD_DIR}/, $(addsuffix .o, ${OBJ_SRC}))

INSTALLED_LIB=${PREFIX}/lib/libsxupdate.a
INSTALLED_HEADERS=$(addprefix ${PREFIX}/include/sxupdate/, $(addsuffix .h, api))

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
	@echo Built $@


${OBJS}: ${BUILD_DIR}/%.o : %.c
	@mkdir -p `dirname "$@"`
	${CC} ${CFLAGS} -c $< -o $@

clean:
	rm -f ${OBJS} ${INSTALLED_LIB} ${INSTALLED_HEADERS}

.PHONY: build clean

# build: build_dir ${OBJS}

# build_dir:
# 	mkdir -p ${BD}

# ${OBJS} : ${BD}/%.o: %.c
