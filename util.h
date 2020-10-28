#ifndef _UTIL_H_
#define _UTIL_H_

#ifndef __dead
	#ifdef __linux__
    		#include <bsd/stdlib.h>
    		#define __dead __attribute__((noreturn))
	#endif
	#ifdef __FreeBSD__
		#define __dead __dead2
	#endif
#endif

#ifdef __OpenBSD__
	#define PID_MAX 99999	/* since 6.5 */
#endif
#ifdef __FreeBSD__
	#define PID_MAX 99999
#endif

__dead void	die(const char *, ...);
void		*ecalloc(size_t nmemb, size_t size);
int		estrtonum(char *string);

#endif
