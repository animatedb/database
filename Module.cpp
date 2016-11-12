/*
* Module.cpp
*
*  Created on: Sept. 28, 2015
*  \copyright 2015 DCBlaha.  Distributed under the Mozilla Public License 2.0.
*/

#include "Module.h"

#ifdef __linux__
#include <dlfcn.h>
#endif

bool Module::open(char const *fileName)
    {
    close();
#if(USE_GLIB)
    mLibrary = g_module_open(fileName, G_MODULE_BIND_LAZY);
#else
#ifdef __linux__
    mLibrary = dlopen(fileName, RTLD_LAZY);
#else
    mModule = LoadLibraryA(fileName);
#endif
#endif
    return (mModule != nullptr);
    }

void Module::close()
    {
    if(mModule)
        {
#if(USE_GLIB)
        g_module_close(mModule);
#else
#ifdef __linux__
        dlclose(mModule);
#else
        FreeLibrary(mModule);
#endif
#endif
        mModule = nullptr;
        }
    }

void Module::loadModuleSymbol(const char *symbolName, ModuleProcPtr *symbol) const
    {
#if(USE_GLIB)
    if(!g_module_symbol(mModule, symbolName, symbol))
        { *symbol = nullptr; }
#else
#ifdef __linux__
    *symbol = dlsym(mModule, symbolName);
#else
    *symbol = GetProcAddress(mModule, symbolName);
#endif
#endif
    }
