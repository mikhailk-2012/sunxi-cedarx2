
include ../../../Makefile.am.inc
include ../../../config.mk.linux

pkglib_LTLIBRARIES = ../../libcedar_plugin_vd_h264.so

libcedar_plugin_vd_h264_la_SOURCES = h264.c \
									 h264_dec.c \
									 h264_nalu.c \
									 h264_hal.c  \
									 h264_mmco.c 

LOCAL_INCLUDE = -I. \
			 -I../../../base/include \
			 -I../../../common/include \
			 -I../../../vdecoder/include

libcedar_plugin_vd_h264_la_CFLAGS = $(CEDAR_CFLAGS)
libcedar_plugin_vd_h264_la_CFLAGS += $(LOCAL_INCLUDE)

libcedar_plugin_vd_h264_la_LIBADD = -ldl -lcedar_base -lcedar_common -lcedar_vdecoder

libcedar_plugin_vd_h264_la_LDFLAGS = -fPIC -shared $(CEDAR_LDFLAGS) \
             -L./../../.. $(libcedar_plugin_vd_h264_la_LIBADD)






################################################################################

BuildPath      = ./build
ObjectPath     = $(BuildPath)/obj
OutputPath     = ../
Target         = $(OutputPath)$(pkglib_LTLIBRARIES)   ## output target.

ifneq ($(BuildPath),wildcard($(BuildPath)))
a := $(shell mkdir -p $(BuildPath))
endif
ifneq ($(ObjectPath),wildcard($(ObjectPath)))
a := $(shell mkdir -p $(ObjectPath))
endif



################################################################################
## set the source files, object files
################################################################################

## set the source files.          
SourceFiles = $(libcedar_plugin_vd_h264_la_SOURCES)
  

## set the object files.
ObjectFiles = $(addprefix $(ObjectPath)/, $(addsuffix .o ,$(basename $(notdir $(SourceFiles)))))




################################################################################
## set flags for compile and link
################################################################################

## set compile flags
CompileFlags = $(libcedar_plugin_vd_h264_la_CFLAGS) -mfpu=neon -O2 -fPIC -ldl -march=armv7-a


## set link flags
LoadFlags = $(libcedar_plugin_vd_h264_la_LDFLAGS)


################################################################################
## make commands, all/clean/cleanall
################################################################################

## define commands for make, sush as all, clean
.PHONY: all clean
all:$(Target)

clean:
	-rm -f $(ObjectPath)/*
	-rm -rf $(Target)




################################################################################
## define target dependencies.
################################################################################

## compile source files to object files.
$(ObjectFiles):$(ObjectPath)/%.o:%.c
	$(CC) $(CompileFlags) -o $@ -c $<



## link object files to the target share library.
$(Target):$(ObjectFiles)
	$(CC) -o $@ $^ $(LoadFlags)


