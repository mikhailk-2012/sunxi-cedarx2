
include ../Makefile.am.inc
include ../config.mk.linux

pkglib_LTLIBRARIES = libcedar_vencoder.so
 
libcedar_vencoder_la_SOURCES =  BitstreamManager.c \
							   	EncAdapter.c \
							   	FrameBufferManager.c  \
								  venc_device.c \
							   	vencoder.c
 
LOCAL_INCLUDE = -I./include \
             -I../base/include \
             -I../common/include 

libcedar_vencoder_la_CFLAGS = $(CEDAR_CFLAGS)
libcedar_vencoder_la_CFLAGS += $(LOCAL_INCLUDE) 

libcedar_vencoder_la_LIBADD = -ldl -lcedar_base -lcedar_common 

libcedar_vencoder_la_LDFLAGS = $(CEDAR_LDFLAGS) -fPIC -shared $(CEDAR_LDFLAGS) \
             -L./.. $(libcedar_vencoder_la_LIBADD)




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
SourceFiles = $(libcedar_vencoder_la_SOURCES)
  

## set the object files.
ObjectFiles = $(addprefix $(ObjectPath)/, $(addsuffix .o ,$(basename $(notdir $(SourceFiles)))))




################################################################################
## set flags for compile and link
################################################################################

## set the include path for compile flags.
SourceIncludePath = $(LOCAL_INCLUDE)



## set compile flags
CompileFlags = $(libcedar_vencoder_la_CFLAGS) -mfpu=neon -O2 -fPIC -ldl -march=armv7-a


## set link flags
LoadFlags = $(libcedar_vencoder_la_LDFLAGS)


################################################################################
## make commands, all/clean/cleanall
################################################################################

## define commands for make, sush as all, clean
.PHONY: all clean cleantarget cleanall
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


