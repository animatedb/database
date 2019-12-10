/*
* Module.h
*
*  Created on: Sept. 28, 2015
*  \copyright 2015 DCBlaha.  Distributed under the Mozilla Public License 2.0.
*/

#ifndef MODULE_H
#define MODULE_H

#include "Module.h"

#define MODULE_IMPORT extern

#ifdef __linux__
#define MODULE_EXPORT
#else
#define MODULE_EXPORT __declspec(dllexport)
//#include "windef.h"
//#include "WinBase.h"
#include "Windows.h"
#endif


#ifdef __linux__
typedef void *ModuleProcPtr;
#else
typedef FARPROC ModuleProcPtr;
#endif

/// A class that encapsulates a run time or dynamic library.
/// This encapsulates something similar to GLib's GModule.
// This class was originally written to avoid the use of GLib.  This only uses
// Windows functions on MS-Windows. It uses the dl... functions on linux.
// undefined reference to symbol 'dlclose@@GLIBC_2.2.5' on Ubuntu 14-04
class Module
    {
    public:
        Module():
            mModule(nullptr)
            {}
        // This releases the module.
        ~Module()
            { close(); }
        /// @param The file name of the run time library. For both Windows and
        /// linux, a relative filename may search multiple directories.
        bool open(char const *fileName);
        void close();
        /// Load a symbol for the module.
        /// @param symbolName The name of the function/symbol.
        /// @param symbol The pointer to the symbol function.
        void loadModuleSymbol(char const *symbolName, ModuleProcPtr *symbol) const;
        bool isOpen()
            { return(mModule != 0); }

    private:
#ifdef __linux__
        void *mModule;
#else
        HMODULE mModule;
#endif
    };

#endif

