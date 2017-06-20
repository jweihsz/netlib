#STRIP:=/opt/buildroot-gcc463/usr/bin/mipsel-linux-strip
#CC:=/opt/buildroot-gcc463/usr/bin/mipsel-linux-gcc

STRIP:=strip
CC:=gcc
#CFLAGS += -std=gnu99 -Wall -Werror -D_USE_PCRE -D__WW_VERSION__=\"NON\"
CFLAGS += -std=gnu99 -Wall  -D_USE_PCRE -D__WW_VERSION__=\"NON\"
target:=bin_test
objs = $(patsubst %.c,%.o,$(wildcard *.c))

.PHONY:clean all 
all:$(target) 
	$(STRIP) $^
$(target):$(objs) 
	$(CC) -o $(target) $(objs) -ljson-c -lpthread -lpcre

clean:
	rm -f $(target) $(objs)
