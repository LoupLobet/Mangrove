/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <err.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

void *
ecalloc(size_t nmemb, size_t size)
{
	void *p;

	if (!(p = calloc(nmemb, size)))
		err(1, NULL);
	return p;
}
