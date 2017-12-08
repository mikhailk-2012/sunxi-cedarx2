
include ../Makefile.am.inc
include ../config.mk.linux


pkglib_LTLIBRARIES = libcedar_base.so

libcedar_base_la_SOURCES = AwPool.c CdxQueue.c CdxList.c CdxUtils.c 

libcedar_base_la_CFLAGS = -fPIC $(CEDAR_CFLAGS)
LOCAL_INCLUDE = -I./include
libcedar_base_la_CFLAGS += $(LOCAL_INCLUDE)

libcedar_base_la_LDFLAGS = -fPIC -shared $(CEDAR_LDFLAGS)
LOCAL_LIB = -lpthread
libcedar_base_la_LDFLAGS += $(LOCAL_LIB)






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
## set the source files, object files and dependency files
################################################################################

## set the source files.             
SourceFiles = $(libcedar_base_la_SOURCES)
  

## set the object files.
ObjectFiles = $(addprefix $(ObjectPath)/, $(addsuffix .o ,$(basename $(notdir $(SourceFiles)))))




################################################################################
## set flags for compile and link
################################################################################

## set compile flags
CompileFlags = $(libcedar_base_la_CFLAGS) -mfpu=neon -O2 -fPIC -ldl -march=armv7-a


## set link flags
LoadFlags = $(libcedar_base_la_LDFLAGS)


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



