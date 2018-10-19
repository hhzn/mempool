src = $(wildcard *.c)
obj = $(src:.c=.o)

LDFLAGS = -shared
CFLAGS += -fPIC -Wall


all: mempool.so

mempool.so: $(obj)
	gcc $^ $(LDFLAGS) -o $@

${src:.c=.d}:%.d:%.c
	gcc $(CFLAGS)


.PHONY: clean
clean:
	rm -f $(obj) mempool.so