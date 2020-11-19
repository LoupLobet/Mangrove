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
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

#define SWAP(X,Y, BUF)	do { BUF = X; X = Y; Y = BUF; } while (0);

extern char *__progname;
static int vflag, cflag, bflag, rflag, wflag;
static int rval;

static void		cook_args(char **);
static void		cook_link(int, int, FILE *);
static void		cook_stdin(int *, int *, char **, int *);
static void		link_pids(int, int, char *);

int
main(int argc, char *argv[])
{
	int ch;
	char cwd[PATH_MAX];

	if ((getcwd(cwd, sizeof(cwd))) != NULL) {
		if (chdir(cwd) == -1)
			errx(1, "Can't move to current directory: %s", cwd);
	} else {
		errx(1, "Can't fetch current directory");
	}

	while ((ch = getopt(argc, argv, "wvcbr")) != -1) {
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
		case 'r':
			rflag = 1;
			break;
		case 'w':
			wflag = 1;
			break;
		default:
			(void)fprintf(stdout, "usage: %s [-bcrvw] tree [pid ...]\n", __progname);
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
	int pidnb = 0, pid, parent, child;
	char *tpath = NULL;
	const char *errstr = NULL;

	size = 0;
	do {
		if (*argv) {
			if (!strcmp(*argv, "-")) {
				readstdin = 1;
			} else {
				readstdin = 0;
				if (tpath == NULL) {
					tpath = ecalloc(strlen(*argv) + 1, sizeof(char));
					strcpy(tpath, *argv);
				} else {
					pid = strtonum(*argv, INT_MIN, INT_MAX, &errstr);
					if (errstr != NULL)
						errx(1, "Error illegal integer: %s", *argv);
					if (pidnb == 0)
						parent = pid;
					else if (pidnb == 1) {
						child = pid;
						link_pids(parent, child, tpath);
					} else if (pidnb > 0) {
						if (cflag) {
							parent = child;
							child = pid;
						} else {
							child = pid;
						}
						link_pids(parent, child, tpath);
					}
					pidnb++;
				}
			}
			argv++;
		}
		if (readstdin)
			cook_stdin(&parent, &child, &tpath, &pidnb);
	} while (*argv);
	free(tpath);
}

static void
cook_link(int parent, int child, FILE *tp)
{
	int buf;

	if (rflag)
		SWAP(parent, child, buf);
	fprintf(tp, "%d\t%d\n", parent, child);
	if (vflag)
		fprintf(stdout, "%d -> %d\n", parent, child);
	if (bflag) {
		bflag = !bflag;
		rflag = !rflag;
		cook_link(parent, child, tp);
		bflag = !bflag;
		rflag = !rflag;
	}
	if (ferror(stdout))
		warnx("Error in printing on stdout");
}

static void
cook_stdin(int *parent, int *child, char **tree, int *pidnb)
{
	int i;
	int pid;
	char ch;
	char *buf = NULL;
	const char *errstr = NULL;
	FILE *rfd = NULL;

	rfd = stdin;
	for (i = 0; (ch = getc(rfd)) != EOF; i++) {
		if ((buf = realloc(buf, i + 1)) == NULL)
			err(1, NULL);
		if ((ch == ' ') || (ch == '\n')) {
			if (*tree == NULL) {
				*tree = ecalloc(strlen(buf) + 1, sizeof(char));
				strcpy(*tree, buf);
				buf = NULL;
			} else {
				pid = strtonum(buf, INT_MIN, INT_MAX, &errstr);
				if (errstr != NULL)
					errx(1, "Error illegal integer: %s", buf);
				if (*pidnb == 0)
					*parent = pid;
				else if (*pidnb == 1) {
					*child = pid;
					link_pids(*parent, *child, *tree);
				} else if (*pidnb > 0) {
					if (cflag) {
						*parent = *child;
						*child = pid;
					} else {
						*child = pid;
					}
					link_pids(*parent, *child, *tree);
				}
				(*pidnb)++;
			}
			i = -1;
		} else
			buf[i] = ch;
	}
	free(buf);
}

static void
link_pids(int parent, int child, char *tpath)
{
	struct stat fstat;
	FILE *tp = NULL;
	char mode[3];

	/* wflag overwrite links */
	if(wflag)
		strcpy(mode, "w+");
	else
		strcpy(mode, "a+");
	if ((access(tpath, F_OK)) == -1)
			errx(1, "No such file or directory: %s", tpath);
	lstat(tpath, &fstat);
	if (errno == ENOENT)
		errx(1, "Error in obtaining information about file: %s", tpath);
	if (S_ISREG(fstat.st_mode)) {
		if ((tp = fopen(tpath, mode)) != NULL)
			cook_link(parent, child, tp);
		else
			errx(1, "Error in opening file: %s", tpath);
	} else
		errx (1, "Error: %s is not a regular file", tpath);
	fclose(tp);
}
