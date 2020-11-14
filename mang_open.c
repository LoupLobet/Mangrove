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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

extern char *__progname;
static int vflag, rflag;
static int rval;

static void		cook_args(char **);
static void		cook_stdin(void);
static void		open_tree(FILE *, char *);

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

	while ((ch = getopt(argc, argv, "rv")) != -1) {
		switch (ch) {
		case 'v':
			vflag = 1;
			break;
		case 'r':
			rflag = 1;
			break;
		default:
			(void)fprintf(stdout, "usage: %s [-vr] [tree ...]\n", __progname);
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
	FILE *fp;

	fp = stdin;
	do {
		if (*argv) {
			if (!strcmp(*argv, "-"))
				fp = stdin;
			else if ((fp = fopen(*argv, "r")) == NULL) {
				warn("%s", *argv);
				rval = 1;
				++argv;
				continue;
			}
			argv++;
		}
		open_tree(fp, *(argv - 1));
		if (fp == stdin) {
			cook_stdin();
			clearerr(fp);
		}
		else
			(void)fclose(fp);
	} while (*argv);

}

static void
cook_stdin(void)
{
	char ch;
	int i;
	char *filename = NULL;
	FILE *rfd, *wfd;

	rfd = stdin;
	for (i = 0; (ch = getc(rfd)) != EOF; i++) {
		if ((filename = realloc(filename, i + 1)) == NULL)
			die("%s: Error on allocating blocks", __progname);
		if ((ch == ' ') || (ch == '\n')) {
			if ((wfd = fopen(filename, "r")) == NULL)
				die("%s: Error in opening file: %s", __progname, filename);
			open_tree(wfd, filename);
			fclose(wfd);
			i = -1;
		} else
			filename[i] = ch;
	}
}

static void
open_tree(FILE *fp, char *tname)
{
	int ch;

	if (fp != stdin) {
		if ((ch = getc(fp)) != EOF) {
			if (0) {
				fprintf(stdout, "%-10s%-30s%-10s%-10s%-30s\n",
		                  "PPID", "PPNAME", "RELATION", "CPID", "CPNAME");
			}
			if (vflag) {
				fprintf(stdout, "%s:\n", tname);
			}
			do {
				if (ferror(fp))
					warn("%s", __progname);
				clearerr(fp);
				if ((ch == '\t') && !rflag)
					fprintf(stdout, " -> ");
				else
					fprintf(stdout, "%c", ch);
				if (ferror(stdout))
					die("%s: %c: Error on printing on stdout");
			} while ((ch  = getc(fp)) != EOF);
		}
	}
}

