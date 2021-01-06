/* See LICENCE file for copyright and licence details */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/sysctl.h>

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

#define MANGD_CONF_FILE "/etc/mangd.conf"

typedef struct {
	char pargs[_POSIX2_LINE_MAX];
	char cargs[_POSIX2_LINE_MAX];
} CfgLine;

typedef struct Snapshot {
	int entriesnb;
	struct kinfo_proc *kinfo;
	struct Snapshot *next;
	kvm_t *kvmd;
} Snapshot;

extern char *__progname;
useconds_t listening_rate = 10000;

static void 	 diff_procs(void);
static void	 exec_child(Snapshot *, int, FILE *);
static int	 read_config(FILE *, CfgLine *);

int
main(int argc, char *argv[])
{
	if (pledge("exec stdio rpath proc ps", NULL) == -1)
		err(1, "pledge");

	if (argc > 1) {
		(void)fprintf(stdout, "usage: %s [-f msec]\n", __progname);
		return 1;
	}
	diff_procs();
	return 0;
}

static void
diff_procs(void)
{
	char errbuf[_POSIX2_LINE_MAX];
	int i, j;
	int missing;
	FILE *fp;
	Snapshot *ss = NULL;

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
	while (usleep(listening_rate) == 0) {
		if ((ss->next->kinfo = kvm_getprocs(ss->next->kvmd, KERN_PROC_ALL, 0,
	   	   sizeof(struct kinfo_proc), &(ss->next->entriesnb))) == NULL)
			errx(1, "Error can't get procs from kvm");
		/* 
		 * Compute diff between the two snapshot.
		 * Call exec_child() on each diff between s and s->next.
		 */
		for (i = 0; i < ss->entriesnb; i++) {
			for (j = 0, missing = 1; j < ss->next->entriesnb; j++)
				if (ss->kinfo[i].p_pid == ss->next->kinfo[j].p_pid)
					missing = 0;
			if (missing)
				exec_child(ss, i, fp);
		}
		ss = ss->next;
	}
	if (fclose(fp) != 0)
		err(1, NULL);
}

static void
exec_child(Snapshot *ss, int index, FILE *fp)
{
	CfgLine cfg;
	int pid;

	while (read_config(fp, &cfg)) {
		if (!strcmp(cfg.pargs, ss->kinfo[index].p_comm)) {
			pid = fork();
			if (pid == -1)
				errx(1, "Error: can't fork daemon for exec");
			else if (pid == 0) {
				char *cmd[] = { "/bin/sh", "-c", cfg.cargs, NULL };
				if (execvp(cmd[0], cmd) == -1)
					warn(NULL);
			}
		}
	}
	rewind(fp);
}

static int
read_config(FILE *fp, CfgLine *line)
{
	enum { PARENT, CHILD };
	int ch;
	int role;
	int chdsize = 0;
	int prtsize = 0;
	/* flags */
	int escape, comment, quote, linker, error;

	role = PARENT;
	escape = comment = quote = linker = error = 0;
	memset(line->pargs, '\0', sizeof(line->pargs));
	memset(line->cargs, '\0', sizeof(line->cargs));
	while ((ch = fgetc(fp)) != ';') {
		if (ch == EOF)
			return 0;
		if (error)
			continue;
		if (ch == '\n') {
			comment = 0;
			if (quote)
				error = 1;
		} else if (comment)
			continue;
		else if (ch == '\\') {
			escape = 1;
			continue;
		} else if (ch == '\'' && !escape)
			quote = !quote;
		else if (!quote && ch == '#')
			comment = 1;
		else if (!quote && !linker && (ch == ' ' || ch == '\t'))
			continue;
		else if (quote && role == PARENT && prtsize < KI_MAXCOMLEN - 1)
			line->pargs[prtsize++] = ch;
		else if (ch == '-' && !quote)
			linker = 1;
		else if (linker && ch == '>' && role == PARENT && prtsize) {
			role = CHILD;
			linker = 0;
		} else if (quote && role == CHILD && chdsize < _POSIX2_LINE_MAX - 1)
			line->cargs[chdsize++] = ch;
		else
			error = 1;
		escape = 0;
	}
	if (error)
		return 0;
	return 1;
}
