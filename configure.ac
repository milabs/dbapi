AC_INIT([lspapp], [1.0-beta], [matvejchikov@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign dist-bzip2])
AC_CONFIG_MACRO_DIR([m4])

#
# check for programs
#

AC_PROG_CXX
AC_PROG_LIBTOOL

#
# check for boost library components
#

BOOST_REQUIRE([1.46])
BOOST_REGEX()
BOOST_SYSTEM()
BOOST_THREADS()
BOOST_DATE_TIME()
BOOST_FILESYSTEM()

#
# configuration options
#

AC_ARG_ENABLE(debug,
	AS_HELP_STRING([--enable-debug], [enable verbose debugging output]))
if test "x$enable_debug" = "xyes"; then
	CPPFLAGS="-DDEBUG -g -ggdb"
fi

AC_ARG_ENABLE(ccache,
	AS_HELP_STRING([--enable-ccache], [enable caching while compiling]))
if test "x$enable_ccache" = "xyes"; then
	CC="ccache gcc"
	CXX="ccache g++"
fi

AC_CHECK_LIB([curl], [curl_easy_init])

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	src/liboauth/Makefile
])
AC_CONFIG_HEADER([config.h])

# output the results
AC_OUTPUT
