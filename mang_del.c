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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

extern char *__progname;
static int vflag;
static int rval;

static void		cook_args(char **);
static void		cook_stdin(void);
static void		del_tree(char *);

int main(int argc, char *argv[])
{
	int ch;
	char cwd[PATH_MAX];

	if ((getcwd(cwd, sizeof(cwd))) != NULL) {
		if (chdir(cwd) == -1)
			die("%s: Can't move to current directory: %s", __progname, cwd);
	} else {
		die("%s: Can't fetch current directory", __progname);
	}

	while ((ch = getopt(argc, argv, "v")) != -1) {
		switch (ch) {
		case 'v':
			vflag = 1;
			break;
		default :
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
	int readstdin;

	readstdin = 1;
	 do {
	 	if (*argv) {
			if (!strcmp(*argv, "-")) {
				readstdin = 1;
			} else {
				readstdin = 0;
				del_tree(*argv);
			}
			argv++;
		}
		if (readstdin)
			cook_stdin();
	} while (*argv);
}

static void
cook_stdin(void)
{
	int i;
	char ch;
	char *path = NULL;
	FILE *rfd;

	rfd = stdin;
	for (i = 0; (ch = getc(rfd)) != EOF; i++) {
		if ((path = realloc(path, i + 1)) == NULL)
			die("%s: Error on allocating blocks", __progname);
		if ((ch == ' ') || (ch == '\n')) {
			del_tree(path);
			i = -1;
		} else
			path[i] = ch;
	}
}

static void
del_tree(char *path)
{
	struct stat fstat;

	lstat(path, &fstat);
	if (errno == ENOENT)
		die("%s: Error in obtaining information about file: %s", __progname, path);
	if (S_ISREG(fstat.st_mode)) {
		if (remove(path) != 0) {
			die("%s: Error can't remove specified tree: %s", __progname, path);
		} else if (vflag)
			fprintf(stdout, "%s\n", path);
	} else
		die("%s: Error wrong object type: %s is not a regular file", __progname, path);
}
