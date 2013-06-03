lib_LTLIBRARIES = liboauth.la
include_HEADERS = oauth.h 

liboauth_la_SOURCES = oauth.c config.h hash.c xmalloc.c xmalloc.h oauth_http.c
liboauth_la_CFLAGS = -DUSE_BUILTIN_HASH -DHAVE_CURL
