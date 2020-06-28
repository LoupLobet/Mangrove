#include <sys/stat.h>
#include <sys/types.h>

#include <bsd/stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "config.h"

#define MAX_KINSHIPS_NUMBER 256
#define MAX_TREES_NUMBER    64
#define TREE_LINE_SIZE      64
#define PID_MAX_LINK	    64

enum {
	BIDIR, 
	CLEAR, 
	DISABLETREE,
	ENABLETREE,
	KILL, 
	CLINK,
	LINKLISTFORMAT,
	LINKLIST,
	NEWTREE,
	REMOVETREE,
	TREELIST,
	UCHILD,
	ULINK,
	ULINKALL,
	UPARENT,
};

typedef struct {
	int pids[2];
	int action;
	char *tree;
	char *symbol;
} CommandLine;

typedef struct {
	int enable;
	int kinships[MAX_KINSHIPS_NUMBER][2];
	int kinshipsnumber;
	char *name;
} Tree;

static int	clear(char *);
static int	clink(char* ,int, int);
static int	deletetree(char *);
static int	disabletree(char *);
static int	enabletree(char *);
static int	fetchtrees(void);
static int	gettreebyname(char *);
static int	mkill(int);
static int	linklist(char *, char *);
static int 	newtree(char *);
static void	run(void);
static int 	treelist(void);
static void	usage(void);
static int	ulink(char *, int, int);

/* variables */
CommandLine cmd = {{0}, 0, NULL, NULL};
Tree trees[MAX_TREES_NUMBER];
int treesnb = 0;

int
main(int argc, char *argv[])
{
	int i;
	char *ptr;

	if (argc == 1)
		usage();
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-v")) {
			puts("mangrove-"VERSION);
			exit(0);
		/* flags */
		} else if (cmd.action != 0) {
			usage();
		/* actions taking zero argument */
		} else if (!strcmp(argv[i], "-L")) {
			cmd.action = TREELIST;
		} else if ((i + 1 == argc)
		       || (cmd.action != 0)) {
			usage();
		/* actions taking one argument */
		} else if (!strcmp(argv[i], "-d")) {
			cmd.action = DISABLETREE;
			cmd.tree = argv[++i];
		} else if (!strcmp(argv[i], "-e")) {
			cmd.action = ENABLETREE;
			cmd.tree = argv[++i];
		} else if (!strcmp(argv[i], "-k")) {
			cmd.action = KILL;
			if ((cmd.pids[0] = strtol(argv[++i], &ptr, 10)) == 0)
				usage();
		} else if (!strcmp(argv[i], "-m")) {
			cmd.action = LINKLIST;
			cmd.tree = argv[++i];
		} else if (!strcmp(argv[i], "-n")) {
			cmd.action = NEWTREE;
			cmd.tree = argv[++i];
		} else if (!strcmp(argv[i], "-r")) {
			cmd.action = REMOVETREE;
			cmd.tree = argv[++i];
		} else if (!strcmp(argv[i], "-w")) {
			cmd.action = CLEAR;
			cmd.tree = argv[++i];
		} else if ((i + 2 == argc) 
		       || (cmd.action != 0)) {
			usage();
		/* actions taking two arguments */
		} else if (!strcmp(argv[i], "-uc")) {
			cmd.action = UCHILD;
			cmd.tree = argv[++i];
			if ((cmd.pids[0] = strtol(argv[++i], &ptr, 10)) == 0)
				usage();
		} else if (!strcmp(argv[i], "-mf")) {
			cmd.action = LINKLISTFORMAT;
			cmd.tree = argv[++i];
			cmd.symbol = argv[++i];
		} else if (!strcmp(argv[i], "-up")) {
			cmd.action = UPARENT;
			cmd.tree = argv[++i];
			if ((cmd.pids[0] = strtol(argv[++i], &ptr, 10)) == 0)
				usage();
		} else if (!strcmp(argv[i], "-ua")) {
			cmd.action = ULINKALL;
			cmd.tree = argv[++i];
			if ((cmd.pids[0] = strtol(argv[++i], &ptr, 10)) == 0)
				usage();
		} else if ((i + 3 == argc)
		       || (cmd.action != 0)) {
			usage();
		/* actions taking three arguments */
		} else if (!strcmp(argv[i], "-b")) {
			cmd.action = BIDIR;
			cmd.tree = argv[++i];
			if (((cmd.pids[0] = strtol(argv[++i], &ptr, 10)) == 0)
			|| ((cmd.pids[1] = strtol(argv[++i], &ptr, 10)) == 0))
				usage();
		} else if (!strcmp(argv[i], "-l")) {
			cmd.action = CLINK;
			cmd.tree = argv[++i];
			if (((cmd.pids[0] = strtol(argv[++i], &ptr, 10)) == 0)
			|| ((cmd.pids[1] = strtol(argv[++i], &ptr, 10)) == 0))
				usage();
		} else if (!strcmp(argv[i], "-u")) {
			cmd.action = ULINK;
			cmd.tree = argv[++i];
			if (((cmd.pids[0] = strtol(argv[++i], &ptr, 10)) == 0)
			|| ((cmd.pids[1] = strtol(argv[++i], &ptr, 10)) == 0))
				usage();
		} else {
			usage();
		}

	}
	run();
	for (int i = 0; i < treesnb; i++)
		free(trees[i].name);

	return 0;
}

static int
clear(char *treename)
{
	int treeindex;
	FILE *treefile;

	if ((treeindex = gettreebyname(treename) != -1)
	&& (chdir(rootdir) == 0)
	&& (remove(treename) == 0)
	&& ((treefile = fopen(treename, "a")))
	&& (fclose(treefile) == 0))
		return 0;
	else
		puts("mangrove: can't clear specified tree\n");
	return 1;
}

static int
clink(char *treename, int parentpid, int childpid)
{
	int treeindex;
	FILE *treefile;

	/* open the tree file and write link */
	if (((treeindex = gettreebyname(treename)) != -1)
	&& (chdir(rootdir) != -1)
	&& ((treefile = fopen(treename, "a"))))
	{
		fprintf(treefile, "%d%s%d\n", parentpid, linksymbol, childpid);
		/* bidirectional link */
		if (cmd.action == BIDIR)
			fprintf(treefile, "%d%s%d\n", childpid, linksymbol, parentpid);
		fclose(treefile);
		return 0;
	} else {
		printf("mangrove: can't create the specified link\n");
	}
	return 1;
}

static int
deletetree(char *treename)
{
	int treeindex;

	if (((treeindex = gettreebyname(treename)) != -1)
	&& (chdir(rootdir) != -1)
	&& (remove(treename) == 0))
		return 0;
	else
		printf("mangrove: can't delete specified tree: %s\n", treename);
	return 1;
}

static int
disabletree(char * treename)
{
	char *buffer;

	buffer = malloc(1 + strlen(treename));
	sprintf(buffer, ".%s", treename);
	if (remove(buffer) == 0) {
		free(buffer);
		return 0;
	} else {
		free(buffer);
		return 1;
	}
}

static int
enabletree(char * treename)
{
	char *buffer;
	FILE *treefile;
	
	buffer = malloc(1 + strlen(treename));
	sprintf(buffer, ".%s", treename);
	if ((treefile = fopen(buffer, "a"))) {
		free(buffer);
		return 0;
	} else {
		free(buffer);
		return 1;
	}
}

static int
fetchtrees(void)
{
	char linebuffer[TREE_LINE_SIZE];
	struct dirent *dir;
	char *ptr;
	char *buffer;
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
			trees[treesnb].kinshipsnumber = 0;
			buffer = malloc(1 + strlen(trees[treesnb].name));
			/* 
			 * search for a dot file with the same tree name
			 * to know if the tree is enable or not
			 */
			sprintf(buffer, ".%s", trees[treesnb].name);
			if (access(buffer, F_OK) != -1)
				trees[treesnb].enable = 1;
			else
				trees[treesnb].enable = 0;
			/* read and parse the tree */
			tree = fopen(trees[treesnb].name, "r");
			while ((fgets(linebuffer, TREE_LINE_SIZE, tree) != NULL)) {
				trees[treesnb].kinships[trees[treesnb].kinshipsnumber][0]
				= strtol(strtok(linebuffer, linksymbol), &ptr, 10);
				trees[treesnb].kinships[trees[treesnb].kinshipsnumber][1]
				= strtol(strtok(NULL, linksymbol), &ptr, 10);
				trees[treesnb].kinshipsnumber++;
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
gettreebyname(char *treename)
{
	int i;
	int treeindex = -1;

	for (i = 0; i < treesnb; i++) 
		if (!strcmp(trees[i].name, treename))
			treeindex = i;
	return treeindex;
}

static int
mkill(int pid)
{
	int i, j; 


	kill(pid, SIGKILL);
	/* search for pid2kill into trees */
	for (i = 0; i < treesnb; i++) {
		if (trees[i].enable) {
			for (j = 0; j < trees[i].kinshipsnumber; j++)
				if (trees[i].kinships[j][0] == pid) {
					kill(trees[i].kinships[j][1], SIGKILL);
					printf("%d\n", trees[i].kinships[j][1]);
			}
		}
	}
	return 0;
}

static int
linklist(char *treename, char *format)
{
	int i, treeindex;	

	if ((treeindex = gettreebyname(treename)) != -1) {
		for (i = 0; i < trees[treeindex].kinshipsnumber; i++) {
			if (cmd.action == LINKLIST)
				/* use config.h linksymbol string */
				printf("%u%s%u\n", trees[treeindex].kinships[i][0],
				linksymbol, trees[treeindex].kinships[i][1]);
			else if (cmd.action == LINKLISTFORMAT)
				/* use given format (from command line) */
				printf("%u%s%u\n", trees[treeindex].kinships[i][0],
				format, trees[treeindex].kinships[i][1]);
		}
		return 0;
	} else {
		return 1;	
	}
}

static int
newtree(char *treename)
{
	char *buffer;
	FILE *treefile;
	FILE *enable;
	
	buffer = malloc(1 + strlen(treename));
	sprintf(buffer, ".%s", treename);
	if ((chdir(rootdir) != -1)
	&& (treefile = fopen(treename, "a"))
	&& ((enable = fopen(buffer, "a"))))
	{
		free(buffer);
		fclose(treefile);
		fclose(enable);
		return 0;
	} else {
		free(buffer);
		printf("mangrove, can't create new tree: %s\n", treename);
	}
	return 1;
}

static void
run(void)
{
	if (fetchtrees() == 1) {
		puts("mangrove: can't fetch tree(s)\n");
		exit(1);
	}
	switch (cmd.action) {
	case CLEAR :
		exit(clear(cmd.tree));
		break;
	case BIDIR :
		exit(clink(cmd.tree, cmd.pids[0], cmd.pids[1]));
		break;
	case CLINK :
		exit(clink(cmd.tree, cmd.pids[0], cmd.pids[1]));
		break;
	case DISABLETREE :
		exit(disabletree(cmd.tree));
	case ENABLETREE :
		exit(enabletree(cmd.tree));
	case REMOVETREE :
		exit(deletetree(cmd.tree));
		break;
	case KILL :
		exit(mkill(cmd.pids[0]));
	case LINKLIST :
	case LINKLISTFORMAT:
		exit(linklist(cmd.tree, cmd.symbol));
		break;
	case NEWTREE :
		exit(newtree(cmd.tree));
		break;
	case TREELIST :
		exit(treelist());
		break;
	case UCHILD :	/* All U* actions are supported by ulink() */ 
	case ULINK :
	case ULINKALL :
	case UPARENT :
		exit(ulink(cmd.tree, cmd.pids[0], cmd.pids[1]));
		break;
	default :
		break;
	}
	exit(0);
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

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-Lv] [[-b parent child] | [-d tree] | [-k pid] \n"
	        "		    | [-l tree pid1 pid2] | [-m tree] | [-mf tree format] \n"
	        "                    | [-n tree] | [-u tree pid1 pid2] | [-ua tree pid]\n"
	        "                    | [-uc tree pid] | [-up tree pid] | [-w tree]]\n"
	      , getprogname());
	exit(1);
}

static int
ulink(char *treename, int parentpid, int childpid)
{
	char buffer[TREE_LINE_SIZE];
	char *tmptree = "mangroveTmpTree";
	int pids[2];
	int treeindex;
	FILE *inputtree;
	FILE *outputtree;
	char *ptr;

	if ((treeindex = gettreebyname(treename) != -1)
	&& (chdir(rootdir) != -1)
	&& (inputtree = fopen(treename, "r"))
	&& (outputtree = fopen(tmptree, "a")))
	{
		switch (cmd.action) {
		case ULINK :
			while ((fgets(buffer, sizeof(buffer),
			      inputtree) != NULL))
			{
				pids[0] = strtol(strtok(buffer, linksymbol), &ptr, 10);
				pids[1] = strtol(strtok(NULL, linksymbol), &ptr, 10);
				if ((pids[0] == parentpid)
				&& (pids[1] == childpid))
					continue;
				else 
					fprintf(outputtree, "%d%s%d\n",
					pids[0], linksymbol, pids[1]);
			}
			break;
		case UPARENT :
			while ((fgets(buffer, sizeof(buffer),
			      inputtree) != NULL))
			{
				pids[0] = strtol(strtok(buffer, linksymbol), &ptr, 10);
				pids[1] = strtol(strtok(NULL, linksymbol), &ptr, 10);
				if (pids[0] == parentpid)
					continue;
				else
					fprintf(outputtree, "%d%s%d\n",
					pids[0], linksymbol, pids[1]);
			}
			break;
		case UCHILD :
			while ((fgets(buffer, sizeof(buffer), 
			      inputtree) != NULL))
			{
				pids[0] = strtol(strtok(buffer, linksymbol), &ptr, 10);
				pids[1] = strtol(strtok(NULL, linksymbol), &ptr, 10);
				if (pids[1] == parentpid)
					continue;
				else
					fprintf(outputtree, "%d%s%d\n",
					pids[0], linksymbol, pids[1]);
			}
			break;
		case ULINKALL :
			while ((fgets(buffer, sizeof(buffer), 
			      inputtree) != NULL))
			{
				pids[0] = strtol(strtok(buffer, linksymbol), &ptr, 10);
				pids[1] = strtol(strtok(NULL, linksymbol), &ptr, 10);
				if ((pids[0] == parentpid)
				|| (pids[1] == parentpid))
					continue;
				else
					fprintf(outputtree, "%d%s%d\n",
					pids[0], linksymbol, pids[1]);
			}
		}
		fclose(inputtree);
		fclose(outputtree);
		remove(treename);
		rename(tmptree, treename);
		return 0;
	} else {
		printf("mangrove: can't remove specified link(s)\n");
		return 1;
	}
}
