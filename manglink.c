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

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/sysctl.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXIMUM(a, b)	(((a) > (b)) ? (a) : (b))
#define SWAP(X,Y,BUF)	{ BUF = X; X = Y; Y = BUF; }

extern char *__progname;
static int cflag, bflag, rflag;

static void 	 cook_args(char **);
static void 	 write_link(char *, char *);

int
main(int argc, char *argv[])
{
	int ch;

	if (pledge("stdio rpath", NULL) == -1)
		err(1, "pledge");

	while ((ch = getopt(argc, argv, "bcr")) != -1) {
		switch (ch) {
		case 'b':
			bflag = 1;
			break;
		case 'c':
			cflag = 1;
			break;
		case 'r':
			rflag = 1;
			break;
		default:
			(void)fprintf(stdout, "usage: %s [-bcr] [pid ...]\n", __progname);
			exit(1);
		}
	}
	rflag = rflag && !bflag; /* -b override -r */
	argv += optind;
	cook_args(argv);
	return 0;
}

static void
cook_args(char **argv)
{
	char child[KI_MAXCOMLEN];
	char parent[KI_MAXCOMLEN];
	struct stat sbuf;
	int bsize;
	int pidc = 0;
	char *buf = NULL;
	char *tok = NULL;

	if ((*argv == NULL) || ((*argv)[0] == '-')) {
		if (fstat(fileno(stdin), &sbuf) == -1)
			err(1, "stdin");
		bsize = MAXIMUM(sbuf.st_blksize, BUFSIZ);
		if ((buf = malloc(bsize)) == NULL)
			err(1, "malloc");
		if ((buf = fgets(buf, bsize, stdin)) != NULL) {
			for (tok = strtok(buf, " \t\n"); tok != NULL;
			     tok = strtok(NULL, " \t\n")) {
				/* 
				 * The token is assumed to be truncated if the
				 * token exceeds the size of KI_MAXCOMLEN.
				 */
				strlcpy(child, tok, sizeof(child));
				if (pidc++ == 0)
					strlcpy(parent, child, sizeof(parent));
				else 
					write_link(parent, child);
				if (cflag)
					strlcpy(parent, child, sizeof(parent));
			}
		}
		free(buf);
	} else {
		for (; *argv; argv++) {
			strlcpy(child, *argv, sizeof(child));
			if (pidc++ == 0) {
				strlcpy(parent, child, sizeof(parent));
			} else 
				write_link(parent, child);
			if (cflag)
				strlcpy(parent, child, sizeof(parent));
		}
	}
}

static void
write_link(char *parent, char *child)
{
	int i;
	char *buf = NULL;
	char **p = &parent;
	char **c = &child;

	if (rflag)
		SWAP(*p, *c, buf);
	for (i = 0; i < (1 + bflag); i++) {
		(void)fprintf(stdout, "'%s'->'%s'\n", *p, *c);
		if (ferror(stdout))
			err(1, "stdout");
		if (bflag || rflag) 
			SWAP(*p, *c, buf);
	}
}
