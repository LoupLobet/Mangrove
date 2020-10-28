/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

__dead void die(const char *f, ...)
{
	va_list ap;

	va_start(ap, f);
	vfprintf(stderr, f, ap);
	va_end(ap);
	if (f[0] && f[strlen(f) - 1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	} else {
		fputc('\n', stderr);
	}
	exit(1);
}

void *
ecalloc(size_t nmemb, size_t size)
{
	void *p;

	if (!(p = calloc(nmemb, size)))
		die("calloc:");
	return p;
}

int
estrtonum(char *string)
{
	int castedint;
	const char *errstr;
	
	castedint = strtonum(string, INT_MIN, INT_MAX, &errstr);
	if (errstr != NULL)
		die("mang: illegal integer: %s", string);
	return castedint;
}
