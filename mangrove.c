/* See LICENSE for license details. */
#include <sys/cdefs.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "util.h"
#include "config.h"

enum { ULINK, UPARENT, UCHILD, ULINKALL }; /* ulink flags */

typedef struct {
	int enable;
	int links[MAX_LINKS_NUMBER][2];
	int linksnumber;
	char *name;
} Tree;

static int	bidir(char *, int, int);
static int	clear(char *);
static int	clink(char* ,int, int);
static int	deletetree(char *);
static int	disabletree(char *);
static int	enabletree(char *);
static int	fetchtrees(void);
static int	gettreebyname(char *);
static int	islink(int, int, int);
static int	mkill(int);
static int	listlink(char *);
static int 	newtree(char *);
static int 	pidstat(char *, int);
static int 	treelist(void);
__dead void	usage(void); /* for linux __dead attribut is provided by util.h */
static int	ulink(char *, int, int, int);

Tree trees[MAX_TREES_NUMBER];
int treesnb = 0;

int
main(int argc, char *argv[])
{
	int i;

	if (argc == 1)
		usage();
	if (fetchtrees() == 1)
		puts("mangrove: can't fetch tree(s)");

	i = 1;
	/* actions taking zero argument */
	if (i + 1 == argc) {
		if (!strcmp(argv[i], "-v"))
			puts("mangrove-"VERSION);
		else if (!strcmp(argv[i], "-L"))
			exit(treelist());
		else
			usage();
	/* actions taking one argument */
	} else if (i + 2 == argc) {
		if (!strcmp(argv[i], "-d"))
			exit(disabletree(argv[i + 1]));
		else if (!strcmp(argv[i], "-e"))
			exit(enabletree(argv[i + 1]));
		else if (!strcmp(argv[i], "-k"))
			exit(mkill(atoi(argv[i + 1])));
		else if (!strcmp(argv[i], "-m"))
			exit(listlink(argv[i + 1]));
		else if (!strcmp(argv[i], "-n"))
			exit(newtree(argv[i + 1]));
		else if (!strcmp(argv[i], "-r"))
			exit(deletetree(argv[i + 1]));
		else if (!strcmp(argv[i], "-w"))
			exit(clear(argv[i + 1]));
		else
			usage();
	/* actions taking two arguments */
	} else if (i + 3 == argc) {
		if (!strcmp(argv[i], "-uc"))
			exit(ulink(argv[i + 1], atoi(argv[i + 2]), 0, UCHILD));
		else if (!strcmp(argv[i], "-up"))
			exit(ulink(argv[i + 1], atoi(argv[i + 2]), 0, UPARENT));
		else if (!strcmp(argv[i], "-ua"))
			exit(ulink(argv[i + 1], atoi(argv[i + 2]), 0, ULINKALL));
		else if (!strcmp(argv[i], "-s"))
			exit(pidstat(argv[i + 1], atoi(argv[i + 2])));
		else
			usage();
	/* actions taking three arguments */
	} else if (i + 4 == argc) {
		if (!strcmp(argv[i], "-b"))
			exit(bidir(argv[i + 1], atoi(argv[i + 2]), atoi(argv[i + 3])));
		else if (!strcmp(argv[i], "-l"))
			exit(clink(argv[i + 1], atoi(argv[i + 2]), atoi(argv[i + 3])));
		else if (!strcmp(argv[i], "-u"))
			exit(ulink(argv[i + 1], atoi(argv[i + 2]), atoi(argv[i + 3]), ULINK));
		else
			usage();
	} else {
		usage();
	}

	/* free trees names */
	for (int i = 0; i < treesnb; i++)
		free(trees[i].name);

	return 0;
}

static int
bidir(char *tname, int parent, int child)
{
	int tindex;
	FILE *tfile;

	/* open the tree and write link */
	if (((tindex = gettreebyname(tname)) != -1)
	&& (chdir(rootdir) != -1)
	&& ((tfile = fopen(tname, "a"))))
	{
		fprintf(tfile, "%d%s%d\n", parent, linksymbol, child);
		fprintf(tfile, "%d%s%d\n", child, linksymbol, parent);
		fclose(tfile);
		return 0;
	} else {
		puts("mangrove: can't create the specified link");
	}
	return 1;
}

static int
clear(char *tname)
{
	int tindex;
	FILE *tfile;

	/* remove given tree an recreate it */
	if ((tindex = gettreebyname(tname) != -1)
	&& (chdir(rootdir) == 0)
	&& (remove(tname) == 0)
	&& ((tfile = fopen(tname, "a")))
	&& (fclose(tfile) == 0))
		return 0;
	else
		puts("mangrove: can't clear specified tree");
	return 1;
}

static int
clink(char *tname, int parent, int child)
{
	int tindex;
	FILE *tfile;

	/* open the tree file and write link */
	if (((tindex = gettreebyname(tname)) != -1)
	&& (chdir(rootdir) != -1)
	&& ((tfile = fopen(tname, "a"))))
	{
		fprintf(tfile, "%d%s%d\n", parent, linksymbol, child);
		fclose(tfile);
		return 0;
	} else {
		puts("mangrove: can't create the specified link");
	}
	return 1;
}

static int
deletetree(char *tname)
{
	int tindex;
	char *dotfile;

	/* remove tree enabeling */
	if (trees[gettreebyname(tname)].enable) {
		dotfile = alloca(strlen(tname) + 1);
		sprintf(dotfile, ".%s", tname);
		if (remove(dotfile) != 0)
			return -1;
	}
	/* remove givern tree */
	if (((tindex = gettreebyname(tname)) != -1)
	&& (chdir(rootdir) != -1)
	&& (remove(tname) == 0))
		return 0;
	puts("mangrove: can't delete specified tree");
	return 1;
}

static int
disabletree(char * tname)
{
	char *buf;

	/* remove the tree enabeling */
	buf = alloca(1 + strlen(tname));
	sprintf(buf, ".%s", tname);
	if (remove(buf) == 0)
		return 0;
	else
		return 1;
}

static int
enabletree(char * tname)
{
	char *buf;
	FILE *tfile;
	
	/* create dotfile name from the tree name */
	buf = alloca(1 + strlen(tname));
	sprintf(buf, ".%s", tname);
	if ((tfile = fopen(buf, "a")))
		return 0;
	else
		return 1;
}

static int
fetchtrees(void)
{
	char linebuf[MAX_LINE_LENGTH];
	struct dirent *dir;
	char *dotfile;
	char *ptr;
	DIR *root;
	FILE *tree;

	if ((root = opendir(rootdir))) {
		/*
		 * If the root directory exists, retrieves all the 
		 * trees there and copies them to C Tree structs.
		 */
		if (chdir(rootdir) == -1)
			return 1;
		while ((dir = readdir(root)) != NULL) {
			if (dir->d_name[0] == '.')
				continue;
			trees[treesnb].name = malloc(strlen(dir->d_name));
			strcpy(trees[treesnb].name, dir->d_name);
			trees[treesnb].linksnumber = 0;
			dotfile = alloca(1 + strlen(trees[treesnb].name));
			/* 
			 * search for a dot file with the same tree name
			 * to know if the tree is enable or not
			 */
			sprintf(dotfile, ".%s", trees[treesnb].name);
			if (access(dotfile, F_OK) != -1)
				trees[treesnb].enable = 1;
			else
				trees[treesnb].enable = 0;
			/* read and parse the tree */
			tree = fopen(trees[treesnb].name, "r");
			while ((fgets(linebuf, MAX_LINE_LENGTH, tree) != NULL)) {
				trees[treesnb].links[trees[treesnb].linksnumber][0]
				= strtol(strtok(linebuf, linksymbol), &ptr, 10);
				trees[treesnb].links[trees[treesnb].linksnumber][1]
				= strtol(strtok(NULL, linksymbol), &ptr, 10);
				trees[treesnb].linksnumber++;
			}
			fclose(tree);
			treesnb++;
		}
		closedir(root);
	} else {
		/* 
		 * If the root directory does not exist, create using the name 
		 * specified in config.h. Then recursively call fetchtrees()
		 */
		if (mkdir(rootdir, 0700) == -1)
			return 1;
		fetchtrees();
	}
	return 0;
}

static int
gettreebyname(char *tname)
{
	/*
	 * gettreebyname() is used to get index 
	 * of a tree in trees[] array from his name.
	 */
	int i;
	int tindex = -1;

	for (i = 0; i < treesnb; i++) 
		if (!strcmp(trees[i].name, tname))
			tindex = i;
	return tindex;
}

static int
islink(int tindex, int parent, int child)
{
	/*
	 * if the link parent -> child exists in 
	 * trees[tindex], return 0, else return 1.
	 */
	int i;

	for (i = 0; i < trees[tindex].linksnumber; i++)
		if ((trees[tindex].links[i][0] == parent)
		&& (trees[tindex].links[i][1] == child))
			return 0;
	return 1;
}

static int
mkill(int pid)
{
	int killed[MAX_TREES_NUMBER * MAX_LINKS_NUMBER];
	int killednb = 0;
	int i, j, k, l; 

	kill(pid, SIGKILL);
	killed[killednb] = pid;
	killednb++;
	for (i = 0; i < treesnb; i++) {
		/* ignore disabled trees */
		if (!trees[i].enable)
			continue;
		for (j = 0; j < trees[i].linksnumber; j++)
			for (k = 0; k < killednb; k++)
			/*
			 * if the link between the last killed 
			 * pid and trees[i].links[j][1] exists
			 */
			/* 
			 * the following block is not indented because
			 * trying to respect 80 characters maximum lines.
			 */
			if (!islink(i, killed[k], trees[i].links[j][1])) {
				kill(trees[i].links[j][1], SIGKILL);
				killed[killednb] = trees[i].links[j][1];
				killednb++;
				memset(trees[i].links[j], 0, 2 * sizeof(int));
				/* 
				 * remove all links in current tree where 
				 * trees[i].links[j][1] is a child
				 */
				for (l = 0; l < trees[i].linksnumber; l++)
					if (trees[i].links[l][1] == trees[i].links[j][1])
						memset(trees[i].links[l], 0, 2 * sizeof(int));
			}
	}
	return 0;
}

static int
listlink(char *tname)
{
	/* display raw infos about the spacified tree */
	int i;
	int tindex;

	if ((tindex = gettreebyname(tname)) != -1) {
		for (i = 0; i < trees[tindex].linksnumber; i++)
			printf("%u%s%u\n", trees[tindex].links[i][0],
			linksymbol, trees[tindex].links[i][1]);
		return 0;
	} else {
		puts("mangrove: can't find specified tree");
		return 1;	
	}
}

static int
newtree(char *tname)
{
	char *buf;
	FILE *enable;
	FILE *tfile;
	
	buf = alloca(1 + strlen(tname));
	sprintf(buf, ".%s", tname);
	if ((chdir(rootdir) != -1)
	&& (tfile = fopen(tname, "a"))
	&& ((enable = fopen(buf, "a"))))
	{
		fclose(tfile);
		fclose(enable);
		return 0;
	} else {
		puts("mangrove, can't create new tree");
	}
	return 1;
}

static int
pidstat(char *tname, int pid)
{
	/* 
	 * give info/status of a given pid,
	 * please see man for return values
	 */
	int ischild = 0;
	int isparent = 0;
	int i;
	int tindex;

	if ((tindex = gettreebyname(tname)) != -1) {
		for(i = 0;
		   (i < trees[tindex].linksnumber);
		   i++)
		{
			if (trees[tindex].links[i][0] == pid)
				isparent = 1;
			if (trees[tindex].links[i][1] == pid)
				ischild = 2;
		}
		printf("%d\n", isparent + ischild);
		return  0;
	}
	return -1;
}

static int
treelist(void)
{
	int i;

	for (i = 0; i < treesnb; i++)
		if (trees[i].enable)
			printf("[*] %s\n", trees[i].name);
		else
			printf("[ ] %s\n", trees[i].name);
	return 0;
}

__dead void
usage(void)
{
	fprintf(stderr, "usage: %s [-Lv]\n", getprogname());
	fprintf(stderr, "usage: %s [ [-b parent child] | [-d tree] | [-e tree] | [-k pid] | \n"
			"              [-l tree pid pid] | [-la pids] | [-m tree] | [-n tree] |\n"
			"              [-r tree] | [-s tree pid] | [-u tree pid pid] | [-ua tree pid] |\n"
			"              [-uc tree pid] | [-up tree pid] | [-w tree] ]\n"
	      , getprogname());
	exit(1);
}

static int
ulink(char *tname, int parent, int child, int flag)
{
	/* 
	 * this function handle ulink uparent uchild ulinkall
	 * functions using flag value (see declaration for flag enum).
	 */
	char filebuf[MAX_LINKS_NUMBER * MAX_LINE_LENGTH];
	char linebuf[MAX_LINE_LENGTH];
	char linecpy[MAX_LINE_LENGTH];
	int pids[2];
	char *ptr;
	FILE *tfile;
	
	/* copy tree into a string */
	if ((chdir(rootdir) != -1)
	&& ((tfile = fopen(tname, "r")))) {
		while (fgets(linebuf, MAX_LINE_LENGTH, tfile)) {
			strcpy(linecpy, linebuf);
			pids[0] = strtol(strtok(linecpy, linksymbol), &ptr, 10);
			pids[1] = strtol(strtok(NULL, linksymbol), &ptr, 10);
			/* filters */
			switch (flag) {
			case ULINK:
				if (!((pids[0] == parent)
				&& (pids[1] == child)))
					strcat(filebuf, linebuf);
				break;
			case UPARENT:
				if (pids[0] != parent)
					strcat(filebuf, linebuf);
				break;
			case UCHILD:
				if (pids[1] != parent)
					strcat(filebuf, linebuf);
				break;
			case ULINKALL:
				if ((pids[0] != parent)
				&& (pids[1] != parent))
					strcat(filebuf, linebuf);
				break;
			}

		}
		fclose(tfile);
		/* write filtered tree */
		if ((tfile = fopen(tname, "w"))) {
			fprintf(tfile, "%s", filebuf);
			fclose(tfile);
			return 0;
		}
	}
	puts("mangrove: can't remove specified link(s)");
	return 1;
}
