#ifdef _MSC_VER
# pragma once
#endif
#ifndef EXPORT_MACRO_H
#define EXPORT_MACRO_H
#    if defined (_WIN32)
#        if defined (EXPORT_MACRO)
#		    define EXPORT_MACRO_DEFINE __declspec(dllexport)
#		else
#		    define EXPORT_MACRO_DEFINE __declspec(dllimport)
#		endif
#	else
#   	    define EXPORT_MACRO_DEFINE __attribute__((visibility("default")))
#	endif
#endif /* EXPORT_MACRO_H */
