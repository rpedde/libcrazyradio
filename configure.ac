# Required initializer
AC_PREREQ(2.59)
AC_INIT([libcrazyradio], [1.0.0], [ron@pedde.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_SRCDIR([src/crazyradio.c])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_C_INLINE

# Checks for programs.
AM_PROG_AR
AC_PROG_CC
AC_PROG_CC_STDC
AC_PROG_LIBTOOL

PKG_CHECK_MODULES([USB], [libusb-1.0])

AC_SUBST([USB_CFLAGS])
AC_SUBST([USB_LIBS])

AC_OUTPUT(Makefile src/Makefile)
