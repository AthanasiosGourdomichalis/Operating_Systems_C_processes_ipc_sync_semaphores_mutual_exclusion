CC = gcc
CFLAGS = -pthread -g
TARGETS = multiproc_1 multiproc_2

all: $(TARGETS)

$(TARGETS): % : %.c util.c util.h
	$(CC) $(CFLAGS) $^ -o $@ > $@_make.log

clean:
	rm -f $(TARGETS)
