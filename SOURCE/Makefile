CROSS_COMPILE ?= /media/Sky-soft/a20/gcc-linaro-arm-linux-gnueabihf-4.7/bin/arm-linux-gnueabihf-

export CC    = $(CROSS_COMPILE)gcc
export CPP    = $(CROSS_COMPILE)g++
export LD    = $(CROSS_COMPILE)gcc
export STRIP = $(CROSS_COMPILE)strip
#export AR     = $(CROSS_COMPILE)-ar

compiled_libraries =  ./*.so
lib_archive = lib-cedarx2.tar.gz
allwiner-blobs = $(wildcard ./../media-codec-lib/lib32/lgnueabihf/*.so)

target_lib_path = ./usr/local/lib/cedarx
target_ldconf_path = ./etc/ld.so.conf.d

pack: strip
	mkdir -p $(target_lib_path)
	mkdir -p $(target_ldconf_path)
	echo "# Cedarx configuration\n/usr/local/lib/cedarx" > $(target_ldconf_path)/cedarx.conf
	cp $(compiled_libraries) $(target_lib_path)/
	cp $(allwiner-blobs) $(target_lib_path)/
	tar -caf $(lib_archive) $(target_lib_path)/*.so $(target_ldconf_path)/cedarx.conf
	rm -fr ./usr	 
	rm -fr ./etc

strip: all
	$(STRIP) -s $(compiled_libraries)

all:
	make -C base
	make -C common
	make -C vdecoder
	make -C vencoder
	make -C plugin
	make -C omxil

clean:
	make clean -C base
	make clean -C common
	make clean -C vdecoder
	make clean -C vencoder
	make clean -C plugin
	make clean -C omxil
	rm -f ./*.so
#	rm -f ./*.tar.gz
