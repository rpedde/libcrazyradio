lib_LTLIBRARIES = libcrazyradio.la
noinst_PROGRAMS = rx-test tx-test

libcrazyradio_la_SOURCES = crazyradio.c
libcrazyradio_la_LIBS = @USB_LIBS@
libcrazyradio_la_CFLAGS = @USB_CFLAGS@

rx_test_SOURCES = rx-test.c
tx_test_SOURCES = tx-test.c

rx_test_LDADD = libcrazyradio.la @USB_LIBS@
tx_test_LDADD = libcrazyradio.la @USB_LIBS@
