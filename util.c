/* See LICENSE file for copyright and license details. */
#include <errno.h>
#include <err.h>
#include <stdlib.h>

#include "util.h"

void *
ecalloc(size_t nmemb, size_t size)
{
	void *p;

	if (!(p = calloc(nmemb, size)))
		err(1, NULL);
	return p;
}
