#ifndef _UTIL_H_
#define _UTIL_H_

__dead void	die(const char *, ...);
void		*ecalloc(size_t nmemb, size_t size);
int		estrtonum(char *string);

#endif
