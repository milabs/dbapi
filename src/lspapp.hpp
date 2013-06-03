#ifndef __LSPAPP_HPP__
#define __LSPAPP_HPP__

#include "config.h"

extern "C" {
# include <stdio.h>
# include <stdint.h>
# include <stdlib.h>
# include <signal.h>

# include <curl/curl.h>
# include <liboauth/oauth.h>
}; /* extern "C" */

#include <map>
#include <vector>
#include <string>

#include "boost.hpp"

// описатель модуля для статики/динамики
typedef struct {
	const char * name;
	std::string (* callback)(void *);
	void * handle;
} lsp_module_t;

// структура-описатель статических модулей
extern lsp_module_t gStaticModules[];

#endif /* __LSPAPP_HPP__ */
