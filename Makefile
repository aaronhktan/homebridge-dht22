CC = gcc
CFLAGS = -Wall -std=gnu99 -g -DDEBUG1_
LD = gcc
LDFLAGS = -g -std=gnu99
LDLIBS = 

SRCS = dht.c bcm2835.c
OBJS = dht.o bcm2835.o
TARGETS = dht

all: ${TARGETS}

dht: $(OBJS)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)

%.o: %.c 
	$(CC) $(CFLAGS) -c $< 

%.d: %.c
	gcc -MM -MF $@ $<

-include $(SRCS:.c=.d)

.PHONY: clean
clean:
	rm -f *~ *.d *.o $(TARGETS) 