#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([arpsweep], [0.49], [martin@linux-ip.net])
AC_CONFIG_SRCDIR([arpsweep.h])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h strings.h sys/time.h])
# from the old automake stuff:
AC_CHECK_HEADERS([limits.h string.h unistd.h pcap-bpf.h pcap.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([gettimeofday memset select strchr strerror strtol])

# more from automake <start>
dnl
dnl First find some usable PCAP headers.
dnl

AC_ARG_WITH(libnet_includes,
    [  --with-libnet-includes=DIR    libnet include directory],
    [with_libnet_includes="$withval"],[with_libnet_includes=no])

if test "$with_libnet_includes" != "no"; then
    CFLAGS="${CFLAGS} -I${with_libnet_includes}"
fi

AC_CHECK_HEADERS(libnet.h,, LIBNET="no")
if test "$LIBNET" = "no"; then
    echo ""
    echo "Error: Can't find libnet 1.1.0 or greater. Visit http://www.packetfact ory.net/libnet for the latest version."
    exit;
fi

AC_ARG_WITH(libpcap_includes,
     [  --with-libpcap-includes=DIR   libcap include directory],
     [with_libpcap_includes="$withval"],[with_libpcap_includes=no])

if test "$with_libpcap_includes" != "no"; then
    CFLAGS="${CFLAGS} -I${with_libpcap_includes}"
fi

AC_CHECK_HEADERS(pcap.h,, LIBPCAP="no")
if test "$LIBPCAP" = "no"; then
    echo ""
    echo "Error: Can't find Libpcap. Visit http://www.tcpdump.org for the latest version."
    exit;
fi

# end from automake <end>

CHECK_GNU_MAKE
if test "x$_cv_gnu_make_command" != "x"; then
  MAKE="$_cv_gnu_make_command"
  ALLSOURCES="\$^"
else
  MAKE="make"
  ALLSOURCES="\$(.ALLSRC)"
fi
AC_SUBST(ALLSOURCES)
AC_SUBST(MAKE)

AC_MSG_CHECKING(whether to compile and link debugging code)
AC_ARG_ENABLE(debug,
[  --disable-debug         Do not compile and link debugging code],
[
  if test "$enableval" = "yes" ; then
    AC_MSG_RESULT(yes)
    LDFLAGS="$LDFLAGS -g"
    test -n "$GCC" && CFLAGS="$CFLAGS -Wall"
  else
    AC_MSG_RESULT(no)
  fi
],
[
    AC_MSG_RESULT(yes)
    LDFLAGS="$LDFLAGS -g"
    test -n "$GCC" && CFLAGS="$CFLAGS -Wall"
])

if test -n "$GCC"; then
  dnl if we are running gcc, use -pipe
  test -n "$GCC" && CFLAGS="$CFLAGS -pipe"

  AC_MSG_CHECKING(whether to compile and link profiling code)
  AC_ARG_ENABLE(profiling,
  [  --enable-profiling      Compile and link profiling code],
  [
    if test "$enableval" = "yes" ; then
      AC_MSG_RESULT(yes)
      CFLAGS="$CFLAGS -pg"
      LDFLAGS="$LDFLAGS -g -pg"
    else
      AC_MSG_RESULT(no)
    fi
  ],
  [
    AC_MSG_RESULT(no)
  ])
fi

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
