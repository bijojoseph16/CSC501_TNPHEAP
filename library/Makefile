CFLAGS := -m64 -O2 -g -D_GNU_SOURCE -D_REENTRANT -W -I/usr/local/include
LDFLAGS := -m64 -lm

all: tnpheap.c
	$(CC) $(CFLAGS) -Wall -fPIC -c tnpheap.c
	$(CC) $(CFLAGS) -shared -Wl,-soname,libtnpheap.so.1 -o libtnpheap.so.1.0 tnpheap.o

install: libtnpheap.so.1.0
	cp libtnpheap.so.1.0 /usr/lib/libtnpheap.so.1
	ln -fs /usr/lib/libtnpheap.so.1 /usr/lib/libtnpheap.so
	cp npheap.h  /usr/local/include
	cp include/tnpheap.h  /usr/local/include


clean:
	rm -f *.so *.o *.so.1 *.so.1.0


.PHONY: all clean
