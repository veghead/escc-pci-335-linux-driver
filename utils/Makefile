# Makefile for the ESCC Utilities directory. 

ESCC_DIRS := esccpsettings flushrx flushtx getclock getdsr getfeatures \
		istatus readreg reconly rxready setclock setdtr setfeatures \
		start_timer status stop_timer tronly txactive txadd txleft \
		txtype writecmdr writereg esccclock rdwr ppp

.PHONY : $(ESCC_DIRS) install clean

$(ESCC_DIRS):
	@for dir in $(ESCC_DIRS); do 		\
		$(MAKE) -C $$dir;				\
	done

install:
	rm -rf bin
	mkdir bin
	@for dir in $(ESCC_DIRS); do 		\
		$(MAKE) -C $$dir install;		\
	done
	
clean:
	@for dir in $(ESCC_DIRS); do 		\
		$(MAKE) -C $$dir clean;		\
	done
	rm -rf bin	
