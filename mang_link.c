/*
 * Copyright (c) 2020, Loup Lobet
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/stat.h>
#include <sys/syslimits.h>

#include <dirent.h>
#include <errno.h>
#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

extern char *__progname;
static int vflag, cflag, bflag;
static int rval;

static void		cook_args(char **);
static void		cook_stdin(int **, int *);
static void		link_pids(int *);

int
main(int argc, char *argv[])
{
	int ch;
	char cwd[PATH_MAX];

	if ((getcwd(cwd, sizeof(cwd))) != NULL) {
		if (chdir(cwd) == -1)
			die("%s: Can't move to current directory: %s", __progname, cwd);
	} else {
		die("%s: Can't fetch current directory", __progname);
	}

	while ((ch = getopt(argc, argv, "vcb")) != -1) {
		switch (ch) {
		case 'v':
			vflag = 1;
			break;
		case 'c':
			cflag = 1;
			break;
		case 'b':
			bflag = 1;
			break;
		default:
			(void)fprintf(stdout, "usage: %s [-v] [tree ...]\n", __progname);
			return 1;
		}
	}
	argv += optind;
	cook_args(argv);

	return rval;
}

static void
cook_args(char **argv)
{
	int readstdin = 1;
	int size;
	int *pids = NULL;
	const char *errstr = NULL;

	size = 0;
	do {
		if (*argv) {
			if (!strcmp(*argv, "-")) {
				readstdin = 1;
			} else {
				readstdin = 0;
				if ((pids = realloc(pids, (size + 1) * sizeof(int))) == NULL)
					die("%s: Error on allocating blocks", __progname);
				pids[size] = strtonum(*argv, INT_MIN, INT_MAX, &errstr);
				if (errstr != NULL)
					die("%s: Error illegal integer: %s", __progname, *argv);
				size++;
			}
			argv++;
		}
		if (readstdin)
			cook_stdin(&pids, &size);
	} while (*argv);
}

static void
cook_stdin(int **pids, int *size)
{
	int i;
	char ch;
	char *buf = NULL;
	const char *errstr = NULL;
	FILE *rfd = NULL;

	rfd = stdin;
	for (i = 0; (ch = getc(rfd)) != EOF; i++) {
		if ((buf = realloc(buf, i + 1)) == NULL)
			die("%s: Error on allocating blocks", __progname);
		if ((ch == ' ') || (ch == '\n')) {
			if ((*pids = realloc(*pids, (*size + 1) * sizeof(int))) == NULL)
				die("%s: Error on allocating blocks", __progname);
			(*pids)[*size] = strtonum(buf, INT_MIN, INT_MAX, &errstr);
			if (errstr != NULL)
				die("%s: Error illegal integer: %s", __progname, buf);
			(*size)++;
			i = -1;

		} else
			buf[i] = ch;
	}
}

static void
link_pids(int *pids)
{

}