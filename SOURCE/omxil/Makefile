
include ../Makefile.am.inc
include ../config.mk.linux

pkglib_LTLIBRARIES = libcedar_omxadapter.so
  
libcedar_omxadapter_la_CSOURCES = aw_omx_core.c \
								 transform_color_format.c \
								 aw_registry_table.c
libcedar_omxadapter_la_CPPSOURCES = omx_vdec.cpp  \
								 omx_core_cmp.cpp
 
LOCAL_INCLUDE = -I./include \
			 -I./../base/include \
			 -I./../common/include \
			 -I./../vdecoder/include

libcedar_omxadapter_la_CFLAGS = $(CEDAR_CFLAGS)
libcedar_omxadapter_la_CFLAGS += $(LOCAL_INCLUDE)

libcedar_omxadapter_la_CPPFLAGS = $(CEDAR_CFLAGS)
libcedar_omxadapter_la_CPPFLAGS += $(LOCAL_INCLUDE)

libcedar_omxadapter_la_LIBADD = -ldl -lcedar_base -lcedar_common \
             -lcedar_vdecoder -lpthread

libcedar_omxadapter_la_LDFLAGS = -fPIC -shared $(CEDAR_LDFLAGS) \
             -L./.. $(libcedar_omxadapter_la_LIBADD)


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
CSourceFiles = $(libcedar_omxadapter_la_CSOURCES)
CppSourceFiles = $(libcedar_omxadapter_la_CPPSOURCES)
  

## set the object files.
CppObjectFiles = $(addprefix $(ObjectPath)/, $(addsuffix .o ,$(basename $(notdir $(CppSourceFiles)))))
CObjectFiles = $(addprefix $(ObjectPath)/, $(addsuffix .o ,$(basename $(notdir $(CSourceFiles)))))



################################################################################
## set flags for compile and link
################################################################################

## set compile flags
CCompileFlags = $(libcedar_omxadapter_la_CFLAGS) -mfpu=neon -O2 -fPIC -ldl -march=armv7-a
CPPCompileFlags = $(libcedar_omxadapter_la_CPPFLAGS) -mfpu=neon -O2 -fPIC -ldl -march=armv7-a


## set link flags
LoadFlags = $(libcedar_omxadapter_la_LDFLAGS)


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
$(CppObjectFiles):$(ObjectPath)/%.o:%.cpp
	$(CPP) $(CPPCompileFlags) -o $@ -c $<
$(CObjectFiles):$(ObjectPath)/%.o:%.c
	$(CC) $(CCompileFlags) -o $@ -c $<

## link object files to the target share library.
$(Target):$(CppObjectFiles) $(CObjectFiles)
	$(CPP) -o $@ $^ $(LoadFlags)

