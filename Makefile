CPPFLAGS+=-DCOMMIT=`git rev-parse --short HEAD`

all : cdp cdata.a

cdp : cdp.c

cdata.a : cdata.o cdata-iter.o cdata-encode.o
	$(AR) rcs $@ $^

test : test.o cdata.a

test.o : test.c | cdp
	$(CC) -DCDATAPARSER_STAGE -E $< | \
		./cdp$(exe) > test-def.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean :
	$(RM) cdp cdp.exe test test.exe test.o test-def.c \
		cdata.a cdata.o cdata-iter.o cdata-encode.o \
