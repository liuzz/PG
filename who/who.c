#include <unistd.h>
#include <fcntl.h>
#include <utmp.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define TIME_FORMAT "%Y-%m-%d %H:%M"
#define	UTMP_SIZE  sizeof(struct utmp)
int main(void)
{
	int fd, num;
	char outstr[50];
	typedef struct utmp str_utmp;
	time_t tm_temp;
	struct utmp *utmp_info = (struct utmp *)malloc(sizeof(struct utmp));
	struct tm *bd_time;

	if (utmp_info == NULL) {
		printf("%ld\n", bd_time);
		return -2;
	}

	if ((fd = open("/var/run/utmp", O_RDONLY)) == -1) {
		printf("open error\n");
		return -1;
	}
	while ((num = read(fd, utmp_info, UTMP_SIZE)) && num != -1) {
		if (utmp_info->ut_type == USER_PROCESS) {
				tm_temp = (time_t)((utmp_info->ut_tv).tv_sec); //ut_time == ut_tv.tv.sec
				bd_time = localtime(&tm_temp); //convert sec to broken-down time
				// converts broken-down time to string according to the format, and stores in outstr	
				strftime(outstr, 50, TIME_FORMAT, bd_time); 
				printf("%s	%s	 %s  %s\n", utmp_info->ut_user, utmp_info->ut_line, outstr,
								utmp_info->ut_host);
		}
	}

	free(utmp_info);
	close(fd);
	return 0;
}
