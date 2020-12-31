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
#define MANGD_CONF_FILE "/etc/mangd.conf"

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
int lflag;
useconds_t RefreshRate = 100000; /* 0.1s */

static int 	 diff_procs(void);
static int	 kill_procs(kvm_t *, Snapshot *, int, FILE *);
static int	 read_config(FILE *, CfgLine *);

int
main(int argc, char *argv[])
{
	int ch;

	if (pledge("stdio rpath proc ps", NULL) == -1)
		err(1, "pledge");

	while ((ch = getopt(argc, argv, "l")) != -1) {
		switch (ch) {
		case 'l':
			lflag = 1;
			break;
		default:
			(void)fprintf(stdout, "usage: %s [-l]\n", __progname);
			return 1;
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
	Snapshot *ss = NULL;

	/* 
	 * create chained list of Snapshot with one a t,
	 * and an othert t + RefreshRate.
	 */
	if ((fp = fopen(MANGD_CONF_FILE, "r")) == NULL)
		errx(1, "could not open config file %s", MANGD_CONF_FILE);
	if (((ss = malloc(sizeof(Snapshot))) == NULL) ||
	   ((ss->next = malloc(sizeof(Snapshot))) == NULL))
		err(1, NULL);
	ss->next->next = ss;

	if ((ss->kvmd = kvm_openfiles(NULL, NULL, NULL,
	   KVM_NO_FILES, errbuf)) == NULL)
		errx(1, "%s", errbuf);
	if ((ss->next->kvmd = kvm_openfiles(NULL, NULL, NULL,
	   KVM_NO_FILES, errbuf)) == NULL)
		errx(1, "%s", errbuf);
	if ((ss->kinfo = kvm_getprocs(ss->kvmd, KERN_PROC_ALL, 0,
	   sizeof(struct kinfo_proc), &(ss->entriesnb))) == NULL)
		errx(1, "Error can't get procs from kvm");

	/* listening loop */
	while (usleep(RefreshRate) == 0) {
		if ((ss->next->kinfo = kvm_getprocs(ss->next->kvmd, KERN_PROC_ALL, 0,
	   	   sizeof(struct kinfo_proc), &(ss->next->entriesnb))) == NULL)
			errx(1, "Error can't get procs from kvm");
		/* 
		 * Compute the difference between the two snapshot.
		 * Call kill_procs() on each diff between s and s->next.
		 */
		for (i = 0; i < ss->entriesnb; i++) {
			for (j = 0, missing = 1; j < ss->next->entriesnb; j++)
				if (ss->kinfo[i].p_pid == ss->next->kinfo[j].p_pid)
					missing = 0;
			if (missing)
				kill_procs(ss->next->kvmd, ss, i, fp);
		}
		ss = ss->next;
	}
	if (fclose(fp) != 0)
		err(1, NULL);
	return 0;
}

static int
kill_procs(kvm_t *kvmd, Snapshot *ss, int index, FILE *fp)
{
	CfgLine cfg;
	int entriesnb;
	struct kinfo_proc *kinfo = NULL;
	int i;

	if ((kinfo = kvm_getprocs(kvmd, KERN_PROC_ALL, 0,
	   sizeof(struct kinfo_proc), &entriesnb)) == NULL)
		errx(1, "Error can't get procs from kvm");
	/* fetch parent and child into cfg.prt and cfg.chd */
	while (read_config(fp, &cfg)) {
		if (!(strcmp(cfg.prt, ss->kinfo[index].p_comm))) {
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
	enum { PARENT, CHILD };
	char ch;
	int role;
	int chdsize = 0;
	int prtsize = 0;
	/* flags */
	int comment = 0;
	int quote = 0;
	int linker = 0;
	int error = 0;

	if (line == NULL)
		errx(1, "Error NULL pointer");
	memset(line->prt, '\0', sizeof(line->prt));
	memset(line->chd, '\0', sizeof(line->chd));
	role = PARENT;
	while ((ch = fgetc(fp)) != ';') {
		if (ch == EOF)
			return 0;
		if (error)
			continue;
		if (ch == '\n') {
			comment = 0;
			if (quote)
				error = 1;
		}
		else if (comment)
			continue;
		else if (ch == '\'')
			quote = !quote;
		else if (!quote && ch == '#')
			comment = 1;
		else if (!quote && !linker && (ch == ' ' || ch == '\t'))
			continue;
		else if (quote && role == PARENT && prtsize < KI_MAXCOMLEN)
			line->prt[prtsize++] = ch;
		else if (ch == '-')
			linker = 1;
		else if (linker && ch == '>' && role == PARENT && prtsize) {
			role = CHILD;
			linker = 0;
		} else if (quote && role == CHILD && chdsize < KI_MAXCOMLEN)
			line->chd[chdsize++] = ch;
		else
			error = 1;
	}
	if (error)
		return 0;
	return 1;
}
