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
#include <errno.h>

#define TIME_START	4
#define TIME_END	12

static const char *optstring = "aAlLh";
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
	unsigned short len; 			
}name_len;

typedef struct color_list {
	char name[128];
	char color[16];
}color_list;

typedef struct dirent *direntp;

static int all_flag;
static int almost_flag;
static int link_flag;
static int human_flag;
static int long_flag;

// TODO: A Hash list should be farther better.
static color_list **color_tab = NULL;

static char *get_color(char *);

static void get_color_conf()
{
	char *config , *ptr;
	int count = 0;
	config = getenv("LS_COLORS");
	while (1) {
		color_tab = realloc(color_tab, (count+1) * sizeof(color_list *));
		color_tab[count] = malloc(sizeof(color_list));
		if (!(ptr = strchr(config, '=')))
			break;
		strncpy(color_tab[count]->name, config, (ptr - config));
		(color_tab[count]->name)[ptr - config] = 0;
		config = ptr + 1;
		ptr = strchr(config, ':');
		strncpy(color_tab[count]->color, config, (ptr - config));
		(color_tab[count]->color)[ptr - config] = 0;
		config = ptr + 1;
		count++;
	}
	color_tab[count] = NULL;
}

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
	if (m & S_ISUID)             // set user ID bit
		rtnmode[XUSR] = 's';

	if (m & S_IRGRP)
		rtnmode[RGRP] = 'r';
	if (m & S_IWGRP)
		rtnmode[WGRP] = 'w';
	if (m & S_IXGRP)
		rtnmode[XGRP] = 'x';
	if (m & S_ISGID)
		rtnmode[XGRP] = 's';

	if (m & S_IROTH)
		rtnmode[ROTH] = 'r';
	if (m & S_IWOTH)
		rtnmode[WOTH] = 'w';
	if (m & S_IXOTH)
		rtnmode[XOTH] = 'x';
	if (m & S_ISVTX)
		rtnmode[XOTH] = 't';
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
	char *color;
	color = get_color(name);
	if (modearray[TYPE] == 'l') {
		fname = (char *)malloc(NAME_MAX * 2);
		char *buf = (char *)malloc(PATH_MAX);
		char *temp = (char *)malloc(PATH_MAX);

		sprintf(fname, "\033[%sm%s\033[0m", color, name);
		strcat(fname, " -> ");
		int count = readlink(name, buf, PATH_MAX);
		buf[count] = 0;
		color = get_color(buf);
		sprintf(temp, "\033[%sm%s\033[0m", color, buf);
		strcat(fname, temp);
		free(buf);
		free(temp);
	} else {
		fname = (char *)malloc(NAME_MAX);
		sprintf(fname, "\033[%sm%s\033[0m", color, name);
	}
	return fname;
}

static int get_stat(char *name, struct stat *infobuf)
{
	if (link_flag)
		return stat(name, infobuf);
	else 
		return lstat(name, infobuf);
}

static char *endswith(char *name)
{
	char *ptr = strrchr(name, '.');
	char *suffix;
	if (ptr) {
		strcpy(suffix, ptr + 1);
		return suffix;
	}
	return ptr;
}

static void search_color_table(char *require, char **color)
{
	color_list **tmp = color_tab;
	for (; (*tmp)->name; tmp++) {
		if (!strcmp((*tmp)->name, require)) {
			*color = (*tmp)->color;
			return;
		}
	}
}

static char *get_color(char *name)
{
	struct stat infobuf;
	char *color;
	char *suffix;
	char *type = malloc(3 * sizeof(char));
	link_flag = 0;
	get_stat(name, &infobuf);	
	color = color_tab[0]->color;
	suffix = endswith(name);
	if (suffix) {
		char star[64] = "*.";
		strcat(star, suffix);
		search_color_table(star, &color);
	}
	if (infobuf.st_mode & S_IXUSR) {
		strcpy(type, "ex");
	}
	if (S_ISDIR(infobuf.st_mode)) {
		strcpy(type, "di");
	}
	if (S_ISLNK(infobuf.st_mode)) {
		strcpy(type, "ln");
	} 
	if ((*type))	
		search_color_table(type, &color);
	free(type);
	return color;
}

static int is_dir(char *name)
{
	struct stat infobuf;
	if (get_stat(name, &infobuf) == -1)
		return -1;
	if (S_ISDIR(infobuf.st_mode))
		return 1;
	return 0;
}

static void print_stat_long(char *name)
{
	char modearray[11];
	char *fname;
	strcpy(modearray, "----------");
	struct stat infobuf;
	get_stat(name, &infobuf);
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

static int mycmp_name_len(const void *p1, const void *p2)
{
	name_len *n1 = *(name_len **)p1;
	name_len *n2 = *(name_len **)p2;
	char *s1 = n1->name;
	char *s2 = n2->name;
	return strcasecmp(s1, s2);
}

static void print_dir_long(name_len **table, int count)
{
	qsort(table, count, sizeof(name_len *), mycmp_name_len);
	int i;
	for (i = 0; i < count; i++) {
		print_stat_long(table[i]->name);
	}
}

static void print_dir_simple(name_len **table, int count, int max)
{
	qsort(table, count, sizeof(name_len *), mycmp_name_len);
	int i, j;
	char *color;
	struct winsize win;
	ioctl(1, TIOCGWINSZ, &win);
	int column = win.ws_col;
	unsigned short itemnum = column / (max + 2);  //num of items per line
	int lines = (count / itemnum) + ((count % itemnum) ? 1 : 0);
	for (i = 0; i < lines; i++) {
		for (j = 0; j < itemnum && (j*lines+i) < count; j++) {
			color = get_color(table[j*lines +i]->name);
			fprintf(stdout, "\033[%sm%-*s\033[0m", color, (max + 2), table[j*lines + i]->name);
		}
		fprintf(stdout, "\n");
	}
}

static void do_list(char *dirname)
{
	int rtn;
	if (!(rtn = is_dir(dirname))) {
		if (long_flag)
			print_stat_long(dirname);    //it is not a dir actually
		else {
			name_len *one = malloc(sizeof(name_len));
			strcpy(one->name, dirname);
			print_dir_simple(&one, 1, strlen(dirname));  //it is not a dir actually
			free(one);
		}
		return;
	} else if (rtn == -1) {
		perror(dirname);
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
		fprintf(stderr, "chdir error\n");
		free(current_pwd);
		free(old_pwd);
		return;
	}

	getcwd(current_pwd, PATH_MAX);

	dirp = opendir(current_pwd);

	//FIXME: To much redundant code in this function!!
	//       Move the code below into another function.
	if (long_flag) {
		while ((dp = readdir(dirp))) {
			if ( *(dp->d_name) != '.' ) {
				dir_item_tab = realloc(dir_item_tab, (count+1) * sizeof(name_len *));
				dir_item_tab[count] = malloc(sizeof(name_len));
				strcpy(dir_item_tab[count++]->name, dp->d_name);   
			}
		}						
		dir_item_tab = realloc(dir_item_tab, (count+1) * sizeof(name_len *));
		dir_item_tab[count++] = NULL;   // There is something boring!!! I just want a NULL to terminate.
		print_dir_long(dir_item_tab, count-1);
	} else {
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
	}
	for (; --count >= 0;)
		free(dir_item_tab[count]);
	free(dir_item_tab);

	closedir(dirp);

	if (chdir(old_pwd)) {
		fprintf(stderr, "chdir error\n");
	}

	free(current_pwd);
	free(old_pwd);
}

static void usage(void)
{
	fprintf(stdout, "ls [-aAlLh] [directory...]\n");
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
	get_color_conf();
	int num_dir = argc - optind;
	argc = optind; 			//It is not the amount of argv any more, just an offset.
	if (num_dir == 0)
		do_list(".");
	while (num_dir--) {
		do_list(*(argv + argc++));
	}
	color_list **tmp = color_tab;
	for (; *tmp;)
		free((*tmp++));	
	free(color_tab);
	return 0;
}
