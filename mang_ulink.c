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

extern char *__progname;
static int vflag, pflag, cflag, zflag;
static int rval;

static void		cook_args(char **);
static void		cook_stdin(int **, int *, char **);
static int		delete_line(int, int, int *, int);
static void		read_link(char *, int *, int *);
static void		ulink_pids(char *, int *, int);

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

	while ((ch = getopt(argc, argv, "vpcz")) != -1) {
		switch (ch) {
		case 'v':
			vflag = 1;
			break;
		case 'p':
			pflag = 1;
			break;
		case 'c':
			cflag = 1;
			break;
		case 'z':
			zflag = 1;
			break;
		default:
			(void)fprintf(stdout, "usage: %s [-vpcz] tree [pid ...]\n", __progname);
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
					if ((pids = realloc(pids, (size + 1) * sizeof(int))) == NULL)
						errx(1, "Error on allocating blocks");
					pids[size] = strtonum(*argv, INT_MIN, INT_MAX, &errstr);
					if (errstr != NULL)
						errx(1, "Error illegal integer: %s", *argv);
					size++;
				}
			}
			argv++;
		}
		if (readstdin)
			cook_stdin(&pids, &size, &tpath);
	} while (*argv);
	ulink_pids(tpath, pids, size);
	free(tpath);
	free(pids);
}

static void
cook_stdin(int **pids, int *size, char **tree)
{
	int i;
	char ch;
	char *buf = NULL;
	const char *errstr = NULL;
	FILE *rfd = NULL;

	rfd = stdin;
	for (i = 0; (ch = getc(rfd)) != EOF; i++) {
		if ((buf = realloc(buf, i + 1)) == NULL)
			errx(1, "Error in allocating blocks");
		if ((ch == ' ') || (ch == '\n')) {
			if (*tree == NULL) {
				*tree = ecalloc(strlen(buf) + 1, sizeof(char));
				strcpy(*tree, buf);
				buf = NULL;
			} else {
				if ((*pids = realloc(*pids, (*size + 1) * sizeof(int))) == NULL)
					errx(1, "Error on allocating blocks");
				(*pids)[*size] = strtonum(buf, INT_MIN, INT_MAX, &errstr);
				if (errstr != NULL)
					errx(1, "Error illegal integer: %s", buf);
				(*size)++;
			}
			i = -1;
		} else
			buf[i] = ch;
	}
	free(buf);
}

static int
delete_line(int parent, int child, int *pids, int size)
{
	int delete = 0;
	int i;

	for (i = 0; i < size; i++) {
		if (cflag && (child == pids[i]))
			delete = 1;
		if (pflag && (parent == pids[i]))
			delete = 1;
		if ((parent == pids[0]) && (child == pids[i + 1]))
			delete = 1;
	}
	return delete;
}


static void
read_link(char *line, int *parent, int *child)
{

	const char *errstr = NULL;
	char *linecpy = NULL;
	char *token = NULL;

	linecpy = ecalloc(strlen(line) + 1, 1);
	strcpy(linecpy, line);

	if ((token = strtok(linecpy, "\t")) == NULL)
		errx(1, "Error invalid separator in line: \"%s\"", line);
	*parent = strtonum(token, INT_MIN, INT_MAX, &errstr);
	if (errstr != NULL)
		errx(1, "Error invalid pid: %s", token);
	if ((token = strtok(NULL, "\n")) == NULL)
		errx(1, "Error new line character missing at end of line: \"%s\"", line);
	*child = strtonum(token, INT_MIN, INT_MAX, &errstr);
	if (errstr != NULL)
		errx(1, "Error invalid pid: %s", token);
	free(linecpy);
}

static void
ulink_pids(char * tpath, int *pids, int size)
{
	struct stat fstat;
	int i, parent = 0, child = 0, bufsize = 0;
	char ch;
	char *line = NULL;
	char *buf = NULL;
	FILE *rtp = NULL, *wtp = NULL;

	/* prevent editing an unregular file */
	if ((access(tpath, F_OK)) == -1)
			errx(1, "No such file or directory: %s", tpath);
	lstat(tpath, &fstat);
	if (errno == ENOENT)
		errx(1, "Error in obtaining information about file: %s", tpath);
	if (S_ISREG(fstat.st_mode)) {
		if ((rtp = fopen(tpath, "r")) == NULL)
			errx(1, "Error in opening file for reading: %s", tpath);
		/* 
		  * Read the file (rtp), and write each inchanged lines into a buffer, 
		  * then in the tree file (wtp).
		  */
		for (i = 0; (ch = getc(rtp)) != EOF; i++) {
			if ((line = realloc(line, i + 1)) == NULL)
				errx(1, "Error in allocating blocks");
			if (ch == '\n') {
				line[i] = ch;
				read_link(line, &parent, &child);
				if (!delete_line(parent, child, pids, size)) {
					bufsize += 1;
					if ((buf = realloc(buf, bufsize)) == NULL)
						errx(1, "Error in allocating blocks");
					strcat(buf, line);
				} else if (vflag) {
					fprintf(stdout, "%s", line);
				}
				line = NULL;
				i = -1;
			} else
				line[i] = ch;
		}
		fclose(rtp);
		if ((wtp = fopen(tpath, "w")) == NULL)
			errx(1, "Error in opening file for writing: %s", tpath);
		if (buf)
			fprintf(wtp, "%s", buf);
		fclose(wtp);
		free(line);
		free(buf);
	}

}
