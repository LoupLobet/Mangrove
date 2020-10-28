/* See LICENSE for license details. */
#include <sys/cdefs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/syslimits.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "util.h"
#include "config.h"

enum status { DISABELED, ENABELED };
enum options { NONE, LISTLINKS, LISTTREE, NEWTREE, REMOVETREE, 
	       NEWLINK, REMOVELINK, OPTIONSNB };

extern char *__progname;

int opts[OPTIONSNB];
int vflag, Rflag;

static void	cookargs(char *);
static void	listlinks(char *);
static void	listtree(void);
static void	newlink(char *, int, int);
static void	newtree(char *);
static void	removelink(char *, int, int);
static void	removetree(char *);
static void	setup(void);
static int	treestatus(char *);
__dead void	usage(void);

int
main(int argc, char *argv[])
{
	int ch;

	setup();
	while ((ch = getopt(argc, argv, "cltrku")) =! -1) {
		switch (ch) {
		case 'c':
			opts
			


		}
	}
	argv += optind;

	return 0;
}

static void
cookargs(char *)
{
	FILE *;

	fp = stdin;
	filename = "stdin";
	do {
		if (*argv) {
			fp = stdin;
		else if ((fp = fopen(*argv, "r")) == NULL) {
			die("mang: can't read argument %s", *argv);
			++argv;
			continue;
		} 
		
	}
}

static void
listlinks(char *treename)
{
	int ch;
	FILE *treefile;

	if ((treefile = fopen(treename, "r")) == NULL)
		die("mang: can't read specified tree: %s", treename);
	if ((ch = getc(treefile)) != EOF) {
		fprintf(stdout, "%-10s%-30s%-10s%-10s%-30s\n", 
			"PPID", "PPNAME", "RELATION", "CPID", "CPNAME");
		do {
			if (ferror(treefile))
				die("mang: error in reading : %s", treename);
			clearerr(treefile);
			fprintf(stdout, "%c", ch);
			if (ferror(stdout))
				die("mang: error in printing on stdout");
		} while ((ch = getc(treefile)) != EOF);
	}
}

static void
listtree(void)
{
	DIR *rdir;
	struct dirent *dir;
	int status;
	
	puts("OK");
	/* ./ is because already into rootdir since setup() */
	if ((rdir = opendir("./")) == NULL)
		die("mang: can't open root directory");
	if (((dir = readdir(rdir)) != NULL) 
	&& (dir->d_name[0] != '.')) {
		fprintf(stdout, "%-10s%-20s\n","STATUS", "TREE");
		do {
			if (treestatus(dir->d_name))
				status = ENABELED;
			else
				status = DISABELED;
			fprintf(stdout, "%-10d%-20s\n", status,
				dir->d_name);
		} while (((dir = readdir(rdir)) != NULL)
		  &&    (dir->d_name[0] != '.'));
	}
}

static void
removelink(char *treename, int parent, int child)
{
	FILE *treefiler;
	FILE *treefilew;

	if ((treefiler = fopen(treename, "r")) == NULL)
		die("mang: can't read specified tree: %s", treename);
	if ((treefilew = fopen(treename, "a")) == NULL)
		die("mang: can't write in specified tree: %s", treename);
	fclose(treefiler);
	fclose(treefilew);
}

static void
removetree(char *treename)
{
	char metaname[strlen(treename) + 1];

	sprintf(metaname, ".%s", treename);
	if (remove(treename) != 0)
		die("mang: can't remove specified tree: %s", treename);
	if (remove(metaname) != 0)
		die("mang: can't remove specified tree status %s", metaname);
}	

static void
newtree(char *treename)
{
	FILE *metafile;
	FILE *treefile;
	char metaname[strlen(treename) + 1]; 

	if ((treefile = fopen(treename, "w")) == NULL)
		die("mang: can't create new tree: %s", treename);
	/* fprintf the columns names */
	fclose(treefile);
	sprintf(metaname, ".%s", treename);
	if ((metafile = fopen(metaname, "w")) == NULL)
		die("mang: can't create status file: %s", metaname);
	fclose(metafile);
}

static void
newlink(char *treename, int parent, int child)
{
	FILE *treefile;
	char *parentname = NULL;
	char *childname  = NULL;

	if ((treefile = fopen(treename, "a")) == NULL)
		die("mang: can't open specified tree %s", treename);
	if (parentname == NULL)
		parentname = "\0";
	if (childname == NULL)
		childname = "\0";
	fprintf(treefile, "%-10d%-30s%-10s%-10d%-30s\n", 
		parent, parentname, linksymb, child, childname);
	fclose(treefile);
}

static void
setup(void)
{
	DIR *rdir;

	/* try to create the root dir if it does not exist */
	if ((rdir = opendir(rootdir)) == NULL) {
		if (mkdir(rootdir, 0700) == -1)
			die("mang: can't create root directory");
	}
	if (chdir(rootdir) == -1)
		die("mang: can't move to root directory: %s", rootdir);
	
}

static int
treestatus(char *treename)
{
	char metaname[strlen(treename) + 1];
	int status;
	
	sprintf(metaname, ".%s", treename);
	if (access(metaname, F_OK) != -1)
		status = ENABELED;
	else
		status = DISABELED;
	return status;
}	
	
__dead void
usage(void)
{
	puts("usage");
	exit(1);
}
