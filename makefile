SRCDIR = src/
BLDDIR = bld/
TSTDIR = tst/

LDFLAGS += -shared
CFLAGS += -fPIC -Wall

obj = $(src:.c=.o)
src = $(wildcard ${SRCDIR}*.c)


all: mempool.so

mempool.so: $(obj)
	gcc $^ $(LDFLAGS) -o $(BLDDIR)$@

${src:.c=.d}:%.d:%.c
	gcc $(CFLAGS)


.PHONY: clean
clean:
	rm -f $(obj) mempool.so