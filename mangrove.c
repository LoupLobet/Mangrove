#include <sys/cdefs.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <bsd/stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define __dead __attribute__((noreturn))

#define MAX_KINSHIPS_NUMBER 256
#define MAX_TREES_NUMBER    256
#define TREE_LINE_SIZE      256

typedef struct {
	int enable;
	int links[MAX_KINSHIPS_NUMBER][2];
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
static int	mkill(int);
static int	listlink(char *);
static int	linkall(char *, char *);
static int 	newtree(char *);
static int 	treelist(void);
__dead void	usage(void);
static int	uchild(char *, int);
static int	ulink(char *, int, int);
static int	ulinkall(char *, int);
static int	uparent(char *, int);

Tree trees[MAX_TREES_NUMBER];
int treesnb = 0;

#include "config.h"

int
main(int argc, char *argv[])
{
	int i;

	if (argc == 1)
		usage();
	if (fetchtrees() == 1) {
		puts("mangrove: can't fetch tree(s)\n");
		exit(1);
	}

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
			exit(uchild(argv[i + 1], atoi(argv[i + 2])));
		else if (!strcmp(argv[i], "-up"))
			exit(uparent(argv[i + 1], atoi(argv[i + 2])));
		else if (!strcmp(argv[i], "-ua"))
			exit(ulinkall(argv[i + 1], atoi(argv[i + 2])));
		else if (!strcmp(argv[i], "-la"))
			exit(linkall(argv[i + 1], argv[i + 2]));
		else
			usage();
	/* actions taking three arguments */
	} else if (i + 4 == argc) {
		if (!strcmp(argv[i], "-b"))
			exit(bidir(argv[i + 1], atoi(argv[i + 2]), atoi(argv[i + 3])));
		else if (!strcmp(argv[i], "-l"))
			exit(clink(argv[i + 1], atoi(argv[i + 2]), atoi(argv[i + 3])));
		else if (!strcmp(argv[i], "-u"))
			exit(ulink(argv[i + 1], atoi(argv[i + 2]), atoi(argv[i + 3])));
		else
			usage();
	} else {
		usage();
	}

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
		puts("mangrove: can't create the specified link\n");
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
		puts("mangrove: can't clear specified tree\n");
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
		puts("mangrove: can't create the specified link\n");
	}
	return 1;
}

static int
deletetree(char *tname)
{
	int tindex;
	char *dotfile;

	/* remove givern tree */
	dotfile = malloc(strlen(tname) + 1);
	sprintf(dotfile, ".%s", tname);
	if (((tindex = gettreebyname(tname)) != -1)
	&& (chdir(rootdir) != -1)
	&& (remove(tname) == 0)
	&& (remove(dotfile) == 0))
	{
		free(dotfile);
		return 0;
	} else {
		free(dotfile);
		puts("mangrove: can't delete specified tree\n");
		return 1;
	}
}

static int
disabletree(char * tname)
{
	char *buf;

	/* create dotfile name from the tree name */
	buf = malloc(1 + strlen(tname));
	sprintf(buf, ".%s", tname);
	if (remove(buf) == 0) {
		free(buf);
		return 0;
	} else {
		free(buf);
		return 1;
	}
}

static int
enabletree(char * tname)
{
	char *buf;
	FILE *tfile;
	
	/* create dotfile name from the tree name */
	buf = malloc(1 + strlen(tname));
	sprintf(buf, ".%s", tname);
	if ((tfile = fopen(buf, "a"))) {
		free(buf);
		return 0;
	} else {
		free(buf);
		return 1;
	}
}

static int
fetchtrees(void)
{
	char linebuf[TREE_LINE_SIZE];
	struct dirent *dir;
	char *ptr;
	char *dotfile;
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
			dotfile = malloc(1 + strlen(trees[treesnb].name));
			/* 
			 * search for a dot file with the same tree name
			 * to know if the tree is enable or not
			 */
			sprintf(dotfile, ".%s", trees[treesnb].name);
			if (access(dotfile, F_OK) != -1)
				trees[treesnb].enable = 1;
			else
				trees[treesnb].enable = 0;
			free(dotfile);
			/* read and parse the tree */
			tree = fopen(trees[treesnb].name, "r");
			while ((fgets(linebuf, TREE_LINE_SIZE, tree) != NULL)) {
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
	int i;
	int tindex = -1;

	for (i = 0; i < treesnb; i++) 
		if (!strcmp(trees[i].name, tname))
			tindex = i;
	return tindex;
}

static int
mkill(int pid)
{
	int i, j; 

	kill(pid, SIGKILL);
	/* search for the given pid into the trees */
	for (i = 0; i < treesnb; i++) {
		if (trees[i].enable) {
			for (j = 0; j < trees[i].linksnumber; j++)
				if (trees[i].links[j][0] == pid) {
					ulink(trees[i].name, pid, trees[i].links[j][1]);
					trees[i].links[j][0] = 0;
					printf("%d\n", pid);
					mkill(trees[i].links[j][1]);
					trees[i].links[j][1] = 0;
			}
		}
	}
	return 0;
}

static int
linkall(char *tname, char *pidlst)
{
	char *firstpid;
	char *pid1;
	char *pid2;
	char *ptr;
	
	pid1 = strtok(pidlst, "\n");
	pid2 = strtok(NULL, "\n");
	firstpid = pid1;
	while (pid2 != NULL) {
		bidir(tname, strtol(pid2, &ptr, 10), strtol(pid1, &ptr, 10));
		pid1 = pid2;
		pid2 = strtok(NULL, "\n");
	}
	bidir(tname, strtol(pid1, &ptr, 10), strtol(firstpid, &ptr, 10));
	return 0;
}

static int
listlink(char *tname)
{
	int i;
	int tindex;	

	if ((tindex = gettreebyname(tname)) != -1)
		for (i = 0; i < trees[tindex].linksnumber; i++) {
			printf("%u%s%u\n", trees[tindex].links[i][0],
			linksymbol, trees[tindex].links[i][1]);
		}
	else 
		return 1;	
	return 0;
}

static int
newtree(char *tname)
{
	char *buf;
	FILE *tfile;
	FILE *enable;
	
	buf = malloc(1 + strlen(tname));
	sprintf(buf, ".%s", tname);
	if ((chdir(rootdir) != -1)
	&& (tfile = fopen(tname, "a"))
	&& ((enable = fopen(buf, "a"))))
	{
		free(buf);
		fclose(tfile);
		fclose(enable);
		return 0;
	} else {
		free(buf);
		puts("mangrove, can't create new tree\n");
	}
	return 1;
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
	fprintf(stderr, "usage: %s [-Lv] | [ [-b parent child] | [-d tree] | [-e tree] | [-k pid] | [-l tree pid pid] |\n"
	                "		    [-la \"pids\"] | [-m tree] | [-n tree] | [-r tree] | [-u tree pid pid] |\n"
	                "                    [-ua tree pid] | [-uc tree pid] | [-up tree pid] | [-w tree] ]\n"
	      , getprogname());
	exit(1);
}

static int
uchild(char *tname, int child)
{
	char linebuf[TREE_LINE_SIZE];
	char linecpy[TREE_LINE_SIZE];
	char filebuf[MAX_KINSHIPS_NUMBER * TREE_LINE_SIZE];
	int pids[2];
	char *ptr;
	FILE *tfile;

	/* copy tree into a string */
	if ((chdir(rootdir) != -1)
	&& ((tfile = fopen(tname, "r")))) {
		while (fgets(linebuf, TREE_LINE_SIZE, tfile)) {
			strcpy(linecpy, linebuf);
			pids[0] = strtol(strtok(linecpy, linksymbol), &ptr, 10);
			pids[1] = strtol(strtok(NULL, linksymbol), &ptr, 10);
			/* filter */
			if (pids[1] != child)
				strcat(filebuf, linebuf);
		}
		fclose(tfile);
		/* write filtered tree */
		if ((tfile = fopen(tname, "w"))) {
			fprintf(tfile, "%s", filebuf);
			fclose(tfile);
			return 0;
		}
	}
	puts("mangrove: can't remove specified link(s)\n");
	return 1;
}

static int
ulink(char *tname, int parent, int child)
{
	char linebuf[TREE_LINE_SIZE];
	char linecpy[TREE_LINE_SIZE];
	char filebuf[MAX_KINSHIPS_NUMBER * TREE_LINE_SIZE];
	int pids[2];
	char *ptr;
	FILE *tfile;
	
	/* copy tree into a string */
	if ((chdir(rootdir) != -1)
	&& ((tfile = fopen(tname, "r")))) {
		while (fgets(linebuf, TREE_LINE_SIZE, tfile)) {
			strcpy(linecpy, linebuf);
			pids[0] = strtol(strtok(linecpy, linksymbol), &ptr, 10);
			pids[1] = strtol(strtok(NULL, linksymbol), &ptr, 10);
			/* filter */
			if ((pids[0] != parent)
			&& (pids[1] != child))
				strcat(filebuf, linebuf);
		}
		fclose(tfile);
		/* write filtered tree */
		if ((tfile = fopen(tname, "w"))) {
			fprintf(tfile, "%s", filebuf);
			fclose(tfile);
			return 0;
		}
	}
	puts("mangrove: can't remove specified link(s)\n");
	return 1;
}

static int
ulinkall(char *tname, int parent)
{
	char linebuf[TREE_LINE_SIZE];
	char linecpy[TREE_LINE_SIZE];
	char filebuf[MAX_KINSHIPS_NUMBER * TREE_LINE_SIZE];
	int pids[2];
	char *ptr;
	FILE *tfile;

	/* copy tree into a string */
	if ((chdir(rootdir) != -1)
	&& ((tfile = fopen(tname, "r")))) {
		while (fgets(linebuf, TREE_LINE_SIZE, tfile)) {
			strcpy(linecpy, linebuf);
			pids[0] = strtol(strtok(linecpy, linksymbol), &ptr, 10);
			pids[1] = strtol(strtok(NULL, linksymbol), &ptr, 10);
			/* filter */
			if ((pids[0] != parent)
			|| (pids[1] != parent))
				strcat(filebuf, linebuf);
		}
		fclose(tfile);
		/* write filtered tree */
		if ((tfile = fopen(tname, "w"))) {
			fprintf(tfile, "%s", filebuf);
			fclose(tfile);
			return 0;
		}
	}
	puts("mangrove: can't remove specified link(s)\n");
	return 1;
}

static int
uparent(char *tname, int parent)
{
	char linebuf[TREE_LINE_SIZE];
	char linecpy[TREE_LINE_SIZE];
	char filebuf[MAX_KINSHIPS_NUMBER * TREE_LINE_SIZE];
	int pids[2];
	char *ptr;
	FILE *tfile;

	/* copy tree into a string */
	if ((chdir(rootdir) != -1)
	&& ((tfile = fopen(tname, "r")))) {
		while (fgets(linebuf, TREE_LINE_SIZE, tfile)) {
			strcpy(linecpy, linebuf);
			pids[0] = strtol(strtok(linecpy, linksymbol), &ptr, 10);
			pids[1] = strtol(strtok(NULL, linksymbol), &ptr, 10);
			/* filter */
			if (pids[0] != parent)
				strcat(filebuf, linebuf);
		}
		fclose(tfile);
		/* write filtered tree */
		if ((tfile = fopen(tname, "w"))) {
			fprintf(tfile, "%s", filebuf);
			fclose(tfile);
			return 0;
		}
	}
	puts("mangrove: can't remove specified link(s)\n");
	return 1;
}
