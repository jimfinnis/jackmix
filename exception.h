/**
 * @file exception.h
 * @brief  Brief description of file.
 *
 */

#ifndef __EXCEPTION_H
#define __EXCEPTION_H

#include <string>
#include <stdarg.h>

// currently just a function for building a string out of args.
// One of those horrible C/C++ mismatches, but << is just fugly.

inline std::string _(const char *s,...){
    char buf[1024];
    va_list(args);
    va_start(args,s);
    vsprintf(buf,s,args);
    return std::string(buf);
}
            

#endif /* __EXCEPTION_H */
