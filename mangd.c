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

typedef struct {
	char prt[KI_MAXCOMLEN];
	char chd[KI_MAXCOMLEN];
} CfgLine;

typedef struct {
	int size;
	char **comms;
	int *pids;
} Diff;

typedef struct Snapshot {
	int size;
	int *pids;
	char **comms;
	struct Snapshot *next;
} Snapshot;

extern char *__progname;
int dflag, lflag;
useconds_t usleeptime = 500;

static int 	 diff_procs(kvm_t *);
static int	 get_procs(kvm_t *, int **, char ***);
static int	 kill_procs(kvm_t *, Diff, FILE *);
static int	 read_config(FILE *, CfgLine *);

int
main(int argc, char *argv[])
{
	char errbuf[_POSIX2_LINE_MAX];
	int ch;
	kvm_t *kvmd = NULL;

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
	if ((kvmd = kvm_openfiles(NULL, NULL, NULL,
	   KVM_NO_FILES, errbuf)) == NULL)
		errx(1, "%s", errbuf);
	diff_procs(kvmd);
	return 0;
}

static int
diff_procs(kvm_t *kvmd)
{
	struct snapshot {
		int size;
		int *pids;
		char **comms;
		struct snapshot *next;
	};
	int i, j;
	int missing;
	FILE *fp;
	Snapshot *s = NULL;
	Diff d = { 0, NULL, NULL };

	if ((fp = fopen("/tmp/mang.conf", "r")) == NULL)
		err(1, NULL);
	if (((s = malloc(sizeof(struct snapshot))) == NULL) ||
	   ((s->next = malloc(sizeof(struct snapshot))) == NULL))
		err(1, NULL);
	s->next->next = s;
	s->size = get_procs(kvmd, &(s->pids), &(s->comms));
	while (usleep(usleeptime) == 0) {
		d.size = 0;
		s->next->size = get_procs(kvmd, &(s->next->pids),
				                &(s->next->comms));
		if (((d.pids = realloc(d.pids, s->size * sizeof(int))) == NULL)
		|| ((d.comms = realloc(d.comms, s->size * sizeof(char *))) == NULL))
			err(1, NULL);
		for (i = 0; i < s->size; i++) {
			missing = 1;
			for (j = 0; j < s->next->size; j++) {
				if (s->pids[i] == s->next->pids[j])
					missing = 0;
			}
			if (missing) {
				if ((d.comms[d.size] = realloc(d.comms[d.size],
				   KI_MAXCOMLEN)) == NULL)
					err(1, NULL);
				d.pids[d.size] = s->pids[i];
				strlcpy(d.comms[d.size], s->comms[i], KI_MAXCOMLEN);
				d.size++;
			}
		}
		if (d.size)
			kill_procs(kvmd, d, fp);
		s = s->next;
	}
	if (fclose(fp) != 0)
		err(1, NULL);
	return 0;
}

static int
get_procs(kvm_t *kvmd, int **p_pids, char ***p_comms)
{
	int i, entriesnb = 0;
	struct kinfo_proc *kinfo = NULL;

	if ((kinfo = kvm_getprocs(kvmd, KERN_PROC_ALL, 0,
	   sizeof(struct kinfo_proc), &entriesnb)) == NULL)
		errx(1, "Error can't get procs from kvm");
	if (((*p_pids = realloc(*p_pids, sizeof(int) * entriesnb)) == NULL) ||
	   ((*p_comms = realloc(*p_comms, sizeof(char *) * entriesnb)) == NULL))
		err(1, NULL);
	for (i = 0; i < entriesnb; i++) {
		if (((*p_comms)[i] = realloc((*p_comms)[i], KI_MAXCOMLEN)) == NULL)
			err(1, NULL);
		(*p_pids)[i] = kinfo[i].p_pid;
		strlcpy((*p_comms)[i], kinfo[i].p_comm, KI_MAXCOMLEN);
	}
	return entriesnb;
}

static int
kill_procs(kvm_t *kvmd, Diff d, FILE *fp)
{
	CfgLine cfg;
	int entriesnb;
	struct kinfo_proc *kinfo = NULL;
	int i, j;

	if ((kinfo = kvm_getprocs(kvmd, KERN_PROC_ALL, 0,
	   sizeof(struct kinfo_proc), &entriesnb)) == NULL)
		errx(1, "Error can't get procs from kvm");
	while (read_config(fp, &cfg) == 1) {
		for (i = 0; i < d.size; i++) {
			if (!(strcmp(cfg.prt, d.comms[i]))) {
				for (j = 0; j < entriesnb; j++) {
					if (!strcmp(kinfo[j].p_comm, cfg.chd))
						kill(kinfo[j].p_pid, SIGKILL);
				}
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
	static char buf[1024];
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
