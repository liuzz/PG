#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <pwd.h>
#include <grp.h>
#include <termios.h>
#include <termio.h>

#define TIME_START	4
#define TIME_END	12
static const char *optstring = "aALh";
static const struct option longopts[] = {
	{"all", no_argument, NULL, 'a'},
	{"almost-all", no_argument, NULL, 'A'},
	{"human-readable", no_argument, NULL, 'h'},
	{"long-term", no_argument, NULL, 'l'},
	{"dereference", no_argument, NULL, 'L'},
	{"help", no_argument, NULL, '?'},
	{0,0,0,0}	//the last element of the array has to be filled with zeros
};

enum {
	TYPE, 
	RUSR, WUSR, XUSR,
	RGRP, WGRP, XGRP,
	ROTH, WOTH, XOTH
};

typedef struct name_len {
	char name[NAME_MAX+1]; 			// ctrl + ] linux/limits.h
	unsigned short len; 			// There is no need to save this temporarily.
}name_len;

typedef struct dirent *direntp;

static int all_flag;
static int almost_flag;
static int link_flag;
static int human_flag;
static int long_flag;

static void get_mode(mode_t m, char *rtnmode)
{


	if (S_ISDIR(m))
		rtnmode[TYPE] = 'd';
	if (S_ISCHR(m))
		rtnmode[TYPE] = 'c';
	if (S_ISBLK(m))
		rtnmode[TYPE] = 'b';
	if (S_ISLNK(m))
		rtnmode[TYPE] = 'l';

	if (m & S_IRUSR) 
		rtnmode[RUSR] = 'r';
	if (m & S_IWUSR)
		rtnmode[WUSR] = 'w';
	if (m & S_IXUSR)
		rtnmode[XUSR] = 'x';

	if (m & S_IRGRP)
		rtnmode[RGRP] = 'r';
	if (m & S_IWGRP)
		rtnmode[WGRP] = 'w';
	if (m & S_IXGRP)
		rtnmode[XGRP] = 'x';

	if (m & S_IROTH)
		rtnmode[ROTH] = 'r';
	if (m & S_IWOTH)
		rtnmode[WOTH] = 'w';
	if (m & S_IXOTH)
		rtnmode[XOTH] = 'x';
}

static char *get_user(uid_t uid)
{
	struct passwd *pswd;

	pswd = getpwuid(uid);

	if (pswd == NULL)
		return "NO_USER";

	return pswd->pw_name;
}

static char *get_group(gid_t gid)
{
	struct group *grp;

	grp = getgrgid(gid);

	if (grp == NULL)
		return "NO_GROUP";

	return grp->gr_name;
}

static char *get_time(time_t mtime)
{
	char *rtn;

	rtn = ctime(&mtime); //Wed Jun 30 21:49:08 1993\n
	rtn = rtn + TIME_START;
	rtn[TIME_END] = '\0';

	return rtn;
}

static char *get_name(char *name, char *modearray) 
{
	char *fname;

	if (modearray[TYPE] == 'l') {
		fname = (char *)malloc(PATH_MAX * 2);
		char *buf = (char *)malloc(PATH_MAX);

		strcpy(fname, name);
		strcat(fname, " -> ");
		readlink(name, buf, PATH_MAX);
		strcat(fname, buf);
		free(buf);

		return fname;
	}
	else {
		fname = (char *)malloc(PATH_MAX);
		strcpy(fname, name);

		return fname;
	}
}

static void mystat(char *name, struct stat *infobuf)
{
	if (link_flag)
		stat(name, infobuf);
	else 
		lstat(name, infobuf);
}

static int is_dir(char *name)
{
	struct stat infobuf;
	mystat(name, &infobuf);
	if (S_ISDIR(infobuf.st_mode))
		return 1;
	return 0;
}

static void print_stat(char *name)
{
	char modearray[11];
	char *fname;
	strcpy(modearray, "----------");
	struct stat infobuf;
	mystat(name, &infobuf);
	get_mode(infobuf.st_mode, modearray);
	fprintf(stdout, "%s.", modearray);
	fprintf(stdout, "%4d ", (int)infobuf.st_nlink);			
	fprintf(stdout, "%-5s", get_user(infobuf.st_uid));
	fprintf(stdout, "%-5s", get_group(infobuf.st_gid));
	fprintf(stdout, "%6ld ", (long)infobuf.st_size);
	fprintf(stdout, "%s ", get_time(infobuf.st_mtime));
	fname = get_name(name, modearray);
	fprintf(stdout, "%s\n", fname);
	free(fname);
}

static int mycmp(const void *p1, const void *p2)
{
	name_len *n1 = *(name_len **)p1;
	name_len *n2 = *(name_len **)p2;
	char *s1 = n1->name;
	char *s2 = n2->name;
	return strcasecmp(s1, s2);
}

static void print_dir_simple(name_len **table, int count, int max)
{
	qsort(table, count, sizeof(name_len *), mycmp);
	int i, j;
	struct winsize win;
	ioctl(1, TIOCGWINSZ, &win);
	int column = win.ws_col;
	unsigned short itemnum = column / (max + 3);  // print item's num per line
	int lines = (count / itemnum) + ((count % itemnum) ? 1 : 0);
	for (i = 0; i < lines; i++) {
		for (j = 0; j < itemnum && (i*itemnum+j) < count; j++) {
			printf("%-*s", (max + 3), table[i*itemnum + j]->name);
		}
		printf("\n");
	}
}

static void do_list(char *dirname)
{
	if (!is_dir(dirname)) {
		print_stat(dirname);
		return;
	}
	DIR *dirp;
	direntp dp;
	char *old_pwd = (char *)malloc(PATH_MAX * sizeof(char));
	char *current_pwd = (char *)malloc(PATH_MAX * sizeof(char));
	name_len **dir_item_tab = NULL;
	int count = 0;

	getcwd(current_pwd, PATH_MAX);
	strcpy(old_pwd, current_pwd);

	if (chdir(dirname)) {
		printf("chdir error\n");
		free(current_pwd);
		free(old_pwd);
		return;
	}

	getcwd(current_pwd, PATH_MAX);

	dirp = opendir(current_pwd);

	if (long_flag) {
		while ((dp = readdir(dirp))) {
			if ( *(dp->d_name) != '.' )
				print_stat(dp->d_name);
		}						
	}
	else {
		int max_len = 0;
		while ((dp = readdir(dirp))) {
			if ( *(dp->d_name) != '.' ) {
				dir_item_tab = realloc(dir_item_tab, (count+1) * sizeof(name_len *));
				dir_item_tab[count] = malloc(sizeof(name_len));
				strcpy(dir_item_tab[count]->name, dp->d_name);   
				dir_item_tab[count]->len = strlen(dir_item_tab[count]->name);
				if (dir_item_tab[count++]->len > max_len)
					max_len = dir_item_tab[count - 1]->len;
			}
		}						
		dir_item_tab = realloc(dir_item_tab, (count+1) * sizeof(name_len *));
		dir_item_tab[count++] = NULL;   // There is something boring!!! I just want a NULL to terminate.
		print_dir_simple(dir_item_tab, count-1, max_len);
		//miss collectting for dir_item_tab
		free(dir_item_tab);
	}

	closedir(dirp);

	if (chdir(old_pwd)) {
		printf("chdir error\n");
	}

	free(current_pwd);
	free(old_pwd);
}

static void usage(void)
{
	printf("ls [-aALh][directory...]\n");
}
static void parse_arg(int argc, char *argv[])
{
	int rtn, index;
	while ((rtn = getopt_long(argc, argv, optstring, longopts, &index)) != -1) {
		switch (rtn) {
			case 'a':
				all_flag = 1;
				break;
			case 'A':
				almost_flag = 1;
				break;
			case 'l':
				long_flag = 1;
				break;
			case 'L':
				link_flag = 1;
				break;
			case 'h':
				human_flag = 1;
				break;
			case '?':
				usage();
				exit(0);
		}
	}
}

int main(int argc, char *argv[])
{
	parse_arg(argc, argv);
	int num_dir = argc - optind;
	argc = optind;
	if (num_dir == 0)
		do_list(".");
	while (num_dir--) {
		do_list(*(argv + argc++));
	}
	return 0;
}
