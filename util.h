#ifndef _UTIL_H_
#define _UTIL_H_

/* 
 * manage bsd fonctionality for linux systems, such as
 * getprogname() or __dead prefix for dead functions 
 */
#ifdef __linux__
    #include <bsd/stdlib.h>    		     /* getprogname() for usage() */
    #define __dead __attribute__((noreturn)) /* provide __dead functions */
#endif

/* 
 * arbitrary constants
 */
#define MAX_LINKS_NUMBER 256
#define MAX_TREES_NUMBER 256
#define MAX_LINE_LENGTH  256

/* symbol between pids in tree files */
char *linksymbol = "\t-> ";

#endif
