
SUBDIRS = vdecoder

all:
	make -C $(SUBDIRS)/h264
	make -C $(SUBDIRS)/mjpeg
	make -C $(SUBDIRS)/mpeg2
	make -C $(SUBDIRS)/mpeg4

clean:
	make clean -C $(SUBDIRS)/h264
	make clean -C $(SUBDIRS)/mjpeg
	make clean -C $(SUBDIRS)/mpeg2
	make clean -C $(SUBDIRS)/mpeg4