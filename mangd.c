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
#include <sys/sysctl.h>
#include <sys/stat.h>

#include <errno.h>
#include <err.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CONF_LINE_SIZE 2048
#define CONF_FILE_PATH "/etc/mangd.conf"

typedef struct {
	char prt[KI_MAXCOMLEN];
	char chd[KI_MAXCOMLEN];
} CfgLine;

typedef struct Snapshot {
	int entriesnb;
	struct kinfo_proc *kinfo;
	struct Snapshot *next;
	kvm_t *kvmd;
} Snapshot;

extern char *__progname;
int dflag, lflag;
useconds_t refresh_rate = 100000; /* 0.1s */

static int 	 diff_procs(void);
static int	 kill_procs(kvm_t *, Snapshot *, int, FILE *);
static int	 read_config(FILE *, CfgLine *);

int
main(int argc, char *argv[])
{
	int ch;

	if (pledge("stdio rpath proc ps", NULL) == -1)
		err(1, "pledge");

	while ((ch = getopt(argc, argv, "dil")) != -1) {
		switch (ch) {
		case 'd':
			dflag = 1;
			break;
		case 'l':
			lflag = 1;
			break;
		default:
			(void)fprintf(stdout, "usage: %s [-dl] [-i seconds]\n", __progname);
		}
	}
	diff_procs();
	return 0;
}

static int
diff_procs(void)
{
	char errbuf[_POSIX2_LINE_MAX];
	int i, j;
	int missing;
	FILE *fp;
	Snapshot *s = NULL;

	if ((fp = fopen(CONF_FILE_PATH, "r")) == NULL)
		err(1, NULL);
	if (((s = malloc(sizeof(Snapshot))) == NULL) ||
	   ((s->next = malloc(sizeof(Snapshot))) == NULL))
		err(1, NULL);
	s->next->next = s;

	if ((s->kvmd = kvm_openfiles(NULL, NULL, NULL,
	   KVM_NO_FILES, errbuf)) == NULL)
		errx(1, "%s", errbuf);
	if ((s->next->kvmd = kvm_openfiles(NULL, NULL, NULL,
	   KVM_NO_FILES, errbuf)) == NULL)
		errx(1, "%s", errbuf);
	if ((s->kinfo = kvm_getprocs(s->kvmd, KERN_PROC_ALL, 0,
	   sizeof(struct kinfo_proc), &(s->entriesnb))) == NULL)
		errx(1, "Error can't get procs from kvm");
	while (usleep(refresh_rate) == 0) {
		if ((s->next->kinfo = kvm_getprocs(s->next->kvmd, KERN_PROC_ALL, 0,
	   	   sizeof(struct kinfo_proc), &(s->next->entriesnb))) == NULL)
			errx(1, "Error can't get procs from kvm");
		for (i = 0; i < s->entriesnb; i++) {
			for (j = 0, missing = 1; j < s->next->entriesnb; j++)
				if (s->kinfo[i].p_pid == s->next->kinfo[j].p_pid)
					missing = 0;
			if (missing)
				kill_procs(s->next->kvmd, s, i, fp);
		}
		s = s->next;
	}
	if (fclose(fp) != 0)
		err(1, NULL);
	return 0;
}

static int
kill_procs(kvm_t *kvmd, Snapshot *s, int index, FILE *fp)
{
	CfgLine cfg;
	int entriesnb;
	struct kinfo_proc *kinfo = NULL;
	int i;

	if ((kinfo = kvm_getprocs(kvmd, KERN_PROC_ALL, 0,
	   sizeof(struct kinfo_proc), &entriesnb)) == NULL)
		errx(1, "Error can't get procs from kvm");
	while (read_config(fp, &cfg) == 1) {
		if (!(strcmp(cfg.prt, s->kinfo[index].p_comm))) {
			for (i = 0; i < entriesnb; i++) {
				if (!strcmp(kinfo[i].p_comm, cfg.chd))
					kill(kinfo[i].p_pid, SIGKILL);
			}
		}
	}
	rewind(fp);
	return 0;
}

static int
read_config(FILE *fp, CfgLine *line)
{
	/* 
	 * read_config() reads at most CONF_LINE_SIZE characters from the fp
	 * stream. The readed content is parsed with following rules :
	 * 	- A line have the following prototype : "parent">"child"\n\0
	 * 	- If the line is too long to fit in buf[1024], the daemon is
	 * 	  stopped, and an error message is printed.
	 * 	- Blank lines (i.e., begining with a '\0') are ignored.
	 * 	- Parents and childs have to be double quoted.
	 * 	- No spaces or tabs are expected outside of quotes.
	 * 	- Escape character '\\' can't be used from this same usage
	 * 	  (i.e no double quote(s) inside parent or child quotes).
	 * 	- A line is considered as a comment if it's first character
	 * 	  is a '#', in this case the line will not be parsed.
	 * 	  End of line comments will return an error.
	 * If an Error is encoutered during the parsing, read_config()
	 * return 0 and a warning message is printed. In this case line content
	 * is indeterminate, and should not be used.
	 * Else 1 is returned on successfuk completion.
	 */
	static char buf[CONF_LINE_SIZE];
	int i, j, rval = 1;
	char *p = NULL;

	if (line == NULL)
		errx(1, "Error NULL pointer");
	memset(line->prt, '\0', sizeof(line->prt));
	memset(line->chd, '\0', sizeof(line->chd));
	if ((fgets(buf, sizeof(buf), fp) != NULL) &&
	   ((p = strchr(buf, '\n')) != NULL)) {
		*p = '\0';
		if ((buf[0] == '#') || (buf[0] == '\0'))
			/* comment or balk line, no parsing */
			rval = 0 ;
		else if (buf[0] == '"') {
			for (i = 1; rval && (i <= KI_MAXCOMLEN) &&
			    (buf[i] != '"'); i++)
				line->prt[i - 1] = buf[i];
			if (buf[i++] != '"') {
				warnx("Error quote too long");
				rval = 0;
			}
			if (rval && buf[i++] != '>') {
				warnx("Error unexpected separator");
				rval = 0;
			}
			if (rval && buf[i++] != '"') {
				warnx("Error quote too long");
				rval = 0;
			}
			for (j = 0; rval && (j < KI_MAXCOMLEN) &&
			    (buf[i] != '"'); i++, j++)
				line->chd[j] = buf[i];
			if (rval && buf[i++] != '"') {
				warnx("Error quote too long");
				rval = 0;
			}
			if (rval && &(buf[i]) != p) {
				warnx("Error expected newline after quote");
				rval = 0;
			}
		} else {
			warnx("Error expected quote on first column");
			rval = 0;
		}
	} else {
		if (!feof(fp))
			errx(1, "Error in config file: line too long");
		rval = 0;
	}
	return rval;
}
