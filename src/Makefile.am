SUBDIRS = liboauth

bin_PROGRAMS = lspapp

lspapp_SOURCES = 			\
	lspapp_auth.cpp			\
	lspapp_config.cpp		\
	lspapp_sync.cpp			\
	lspapp_module.cpp		\
	lspapp_storage.cpp		\
	lspapp_storage_dbox.cpp		\
	lspapp_storage_local.cpp	\
	lspapp.cpp

lspapp_CPPFLAGS =			\
	$(BOOST_CPPFLAGS)

lspapp_LDFLAGS =			\
	$(BOOST_REGEX_LDFLAGS)		\
	$(BOOST_SYSTEM_LDFLAGS)		\
	$(BOOST_THREAD_LDFLAGS)		\
	$(BOOST_DATE_TIME_LDFLAGS)	\
	$(BOOST_FILESYSTEM_LDFLAGS)

lspapp_LDADD =				\
	liboauth/.libs/liboauth.a	\
	$(BOOST_REGEX_LIBS)		\
	$(BOOST_SYSTEM_LIBS)		\
	$(BOOST_THREAD_LIBS)		\
	$(BOOST_DATE_TIME_LIBS)		\
	$(BOOST_FILESYSTEM_LIBS)
