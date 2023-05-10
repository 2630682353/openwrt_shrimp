CFLAGS= -Wall -Iinclude -Os -I../libshare/include -I../libipc/src/include
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

all: $(OBJS)
	$(CC) -o portal_cgi $(OBJS) $(CFLAGS) -lshare -lfcgi -lsqlite3

sinclude .depfile
dep:
	$(CC) -MM $(CFLAGS) $(SRCS) > .depfile

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $< 

clean:
	rm -rf .depfile *.o *.so portal_cgi
