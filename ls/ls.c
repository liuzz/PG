#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <pwd.h>
#include <grp.h>

#define TIME_START	4
#define TIME_END	12
static const char *optstring = "aALh";
static const struct option longopts[] = {
		{"all", no_argument, NULL, 'a'},
		{"almost-all", no_argument, NULL, 'A'},
		{"human-readable", no_argument, NULL, 'h'},
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
typedef struct dirent *direntp;
static int all_flag;
static int almost_flag;
static int link_flag;
static int human_flag;

void get_mode(mode_t m, char *rtnmode)
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

char *get_user(uid_t uid)
{
		struct passwd *pswd;

		pswd = getpwuid(uid);

		if (pswd == NULL)
				return "NO_USER";

		return pswd->pw_name;
}

char *get_group(gid_t gid)
{
		struct group *grp;

		grp = getgrgid(gid);

		if (grp == NULL)
				return "NO_GROUP";

		return grp->gr_name;
}

char *get_time(time_t mtime)
{
		char *rtn;
		
		rtn = ctime(&mtime); //Wed Jun 30 21:49:08 1993\n
		rtn = rtn + TIME_START;
		rtn[TIME_END] = '\0';
		
		return rtn;
}

char *get_name(char *name, char *modearray) 
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

void print_stat(char *name)
{
		char modearray[11];
		char *fname;
		strcpy(modearray, "----------");
		struct stat infobuf;
		if (link_flag)
				stat(name, &infobuf);
		else 
				lstat(name, &infobuf);
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

void do_list(char *dirname)
{
		DIR *dirp;
		direntp dp;
		char *old_pwd = (char *)malloc(PATH_MAX * sizeof(char));
		char *current_pwd = (char *)malloc(PATH_MAX * sizeof(char));
		
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
			
		while ((dp = readdir(dirp))) {
				if ( *(dp->d_name) != '.' )
						print_stat(dp->d_name);
		}						

		closedir(dirp);

		if (chdir(old_pwd)) {
				printf("chdir error\n");
		}

		free(current_pwd);
		free(old_pwd);
}

void parse_arg(int argc, char *argv[])
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
						case 'L':
								link_flag = 1;
								break;
						case 'h':
								human_flag = 1;
								break;
						case '?':
								//usage();
								break;
				}
		}
}

int main(int argc, char *argv[])
{
		parse_arg(argc, argv);
		int num_dir = argc - optind;
		argc = optind;
		while (num_dir--) {
				do_list(*(argv + argc++));
		}
		return 0;
}

