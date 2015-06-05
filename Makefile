


all: DEPS
	cd src;make


DEPS:
	cd contrib/AntTweakBar/src; make
	cp contrib/AntTweakBar/lib/* contrib/lib/
	cp contrib/AntTweakBar/include/* contrib/include/
