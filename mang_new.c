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
static mode_t permissions;
static int vflag;
static int rval;

static void		cook_args(char **);
static void		cook_stdin(void);
static void		new_tree(char *);

int main(int argc, char *argv[])
{
	int ch;
	char cwd[PATH_MAX];

	if ((getcwd(cwd, sizeof(cwd))) != NULL) {
		if (chdir(cwd) == -1)
			errx(1, "Can't move to current directory: %s", cwd);
	} else {
		errx(1, "Can't fetch current directory");
	}

	permissions = 0444;
	while ((ch = getopt(argc, argv, "vugo")) != -1) {
		switch (ch) {
		case 'v':
			vflag = 1;
			break;
		case 'u':
			permissions += 0200;
			break;
		case 'g':
			permissions += 0020;
			break;
		case 'o':
			permissions += 0002;
			break;
		default:
			(void)fprintf(stdout, "usage: %s [-vugo] [tree ...]\n", __progname);
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
			if (!strcmp(*argv, "-"))
				readstdin = 1;
			else {
				readstdin = 0;
				new_tree(*argv);
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
			err(1, NULL);
		if ((ch == ' ') || (ch == '\n')) {
			new_tree(path);
			i = -1;
		} else
			path[i] = ch;
	}
	free(path);
}

static void
new_tree(char *path)
{
	char symb[] = "rwxrwxrwx";
	int th;
	int i;

	if (access(path, F_OK) != -1)
		remove(path);
	if ((th = open(path, O_WRONLY | O_CREAT, permissions)) < 0)
		fprintf(stderr, "%s: Error in crating new tree: %s\n", __progname, path);
	else if (vflag) {
		for (i = 0; i < 9; i++)
    			symb[i] = (permissions & (1 << (8-i))) ? symb[i] : '-';
  		symb[9] = '\0';
		fprintf(stdout, "New tree created: %s %s\n", path, symb);
	}
	close(th);
}

