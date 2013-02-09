/*
 * =====================================================================================
 *
 *       Filename:  readproc.c
 *
 *    Description:  
 *    				open /proc, and read /proc/'pid'/stat
 *
 *        Version:  1.0
 *        Created:  12/28/2012 01:12:38 PM
 *       Revision:  none
 *       Compiler:  gcc readproc.c 
 *
 *         Author:  Liu Zhengzhen (), liuzhengzhen9036#gmail#com
 *   Organization:  
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <utmp.h>
#include <ctype.h>

#define PROC_PATH 			"/proc/"

typedef struct proc_t {
	pid_t pid;
	char command[256];
	char status;
	pid_t ppid;
	pid_t pgrp;
	pid_t sid;
	long tty;
	pid_t tpgid;
	unsigned long flags;
	unsigned long min_flt;
	unsigned long cmin_flt;
	unsigned long maj_flt;
	unsigned long cmaj_flt;
	unsigned long long utime;
	unsigned long long stime;
	unsigned long long cutime;
	unsigned long long cstime;
	unsigned long long start_time;
	long priority;
	long nice;
	long alarm;
	int nlwp;
}proc_t;

static int is_all_num(char *ptr) 
{
	for (; *ptr; ptr++) 
		if (!isdigit(*ptr))
			return 0;	
	return 1;
}

static int tty_name2num(char *name) 
{
	char ttyname[32];
	struct stat buffer;
	snprintf(ttyname, 32, "/dev/%s", name);	
	if (stat(ttyname, &buffer) >=0) return buffer.st_rdev;
	snprintf(ttyname, 32, "/dev/pts/%s", name+3);
	if (stat(ttyname, &buffer) >=0) return buffer.st_rdev;
}

int main(void)
{
	DIR *dir;
	struct dirent *diritem;
	FILE *stream;
	struct utmp *user;
	int count = 0;
	int c;
	int n;
	proc_t **proc_stat_table = NULL;
	if (!(dir = opendir(PROC_PATH))) {
		perror("can not open /proc");
		exit(1);
	}
	while (diritem = readdir(dir)) {
		char *dirname = malloc(PATH_MAX);
		struct stat buffer;
		proc_t *proc_stat = malloc(sizeof (proc_t));
		strcpy(dirname, PROC_PATH);
		strcat(dirname, diritem->d_name);
		if (stat(dirname, &buffer)) {
			perror("can not stat");
			exit(2);
		}
		if (!S_ISDIR(buffer.st_mode)) {	// wether it is a dir : stat.h ctrl + ]
			goto final;
		} 
		if (!(is_all_num(diritem->d_name))) {
			goto final;
		}
		strcat(dirname, "/stat");
		if (!(stream = fopen(dirname, "r"))) {
			perror("can not fopen");
			goto final;	
		}
		fscanf(stream, "%ld %s %s " 
				"%ld %ld %ld" 
				"%ld %ld " 
				"%lu %lu %lu %lu "
				"%lu %Lu %Lu %Lu "
				"%Lu %ld "
				"%ld %d %lu "
				"%Lu", 
				&(proc_stat->pid), &(proc_stat->command), &(proc_stat->status), 
				&(proc_stat->ppid), &(proc_stat->pgrp), &(proc_stat->sid),
				&(proc_stat->tty), &(proc_stat->tpgid),
				&(proc_stat->flags), &(proc_stat->min_flt), &(proc_stat->cmin_flt), &(proc_stat->maj_flt), 
				&(proc_stat->cmaj_flt), &(proc_stat->utime), &(proc_stat->stime), &(proc_stat->cutime), 
				&(proc_stat->cstime), &(proc_stat->priority), 
				&(proc_stat->nice), &(proc_stat->nlwp), &(proc_stat->alarm), 
				&(proc_stat->start_time));

		fclose(stream);
		if (proc_stat->tty <= 0) {
			goto final;
		}
		char *temp = strrchr(dirname, '/');
		*temp = 0;
		strcat(dirname, "/cmdline");
		int fd = open(dirname, O_RDONLY);
		if (fd < 0) 	goto final;
		char buf[64];
		n = read(fd, buf, 64);
		for (c = 0; c < n - 1; c++) {
			if (buf[c] == 0) buf[c] = ' ';
		}
		buf[n-1] = 0;
		strcpy(proc_stat->command, buf);	
		proc_stat_table = realloc(proc_stat_table, (count+1) * sizeof (proc_t *));
		if (!proc_stat_table) {
			perror("cannot realloc\n");
			exit(1);
		}
		proc_stat_table[count] = malloc(sizeof(proc_t));
		memcpy(proc_stat_table[count], proc_stat, sizeof(proc_t));
		count++;
final:
		free(dirname);
		free(proc_stat);
		continue;
		//strtoul(dirent->d_name, NULL, 10);		
	}
	utmpname(UTMP_FILE); 			//utmp.h ctrl + ]
	setutent();
	int inter;
	//FIXME: It it is running in X-windows, there will be a segment core.
	while (user=getutent()) {
		proc_t *best = NULL;
		if (user->ut_type != USER_PROCESS) continue;
		long tty_num = tty_name2num(user->ut_line);
		//printf("%s %ld\n", user->ut_user, tty_num);
		for (inter = 0; inter < count; inter++) {
			if (tty_num == proc_stat_table[inter]->tty)
				if (proc_stat_table[inter]->pgrp == proc_stat_table[inter]->tpgid) {
					if (!best) {
						best = malloc(sizeof(proc_t));
						memcpy(best, proc_stat_table[inter], sizeof(proc_t));	
					} else if (proc_stat_table[inter]->start_time > best->start_time){
						memcpy(best, proc_stat_table[inter], sizeof(proc_t));	
					}
				}
		}
		printf("%-*s %-*s %-*s\n", 8, user->ut_user, 8,
									user->ut_line, 10,
									best->command);
		free(best);
	}
	endutent();
	for (; count;)	
		free(proc_stat_table[--count]);
	free(proc_stat_table);
	return 0;
}
