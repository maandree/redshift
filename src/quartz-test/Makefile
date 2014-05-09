.PHONY: all
all: test


ifeq ($(WITH_RANDR),yes)
# Linux
test: fake-quartz.c test.c fake-quartz.h
	$(CC) -g -std=c99 -DENABLE_RANDR -DFAKE_QUARTZ -lxcb-randr -lxcb -o $@ fake-quartz.c test.c

else
# OS X
test: test.c
	$(CC) -g -I/System/Library/Frameworks/ApplicationServices.framework/Versions/A/Frameworks/CoreGraphics.framework/Versions/A/Headers -F/System/Library/Frameworks/ApplicationServices.framework -framework ApplicationServices -std=c99 -o $@ $<

endif

