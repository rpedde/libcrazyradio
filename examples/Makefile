CC         = gcc
CFLAGS     = -std=c99
OUTFLAG    = -o
LIBS       = -lusb-1.0 -lcrazyradio
TARGET     = tx-test rx-test

all:  $(TARGET)

.c.o :
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o *~ TAGS core $(TARGET)

tags:
	etags *.[ch] *.[ch]

tx-test:
	${CC} $(CFLAGS) -o $@ tx-test.c $(CF) $(INCLUDEDIR) $(LIBS)

rx-test:
	${CC} $(CFLAGS) -o $@ rx-test.c $(CF) $(INCLUDEDIR) $(LIBS)
