/*
 * =====================================================================================
 *
 *       Filename:  more.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  12/02/2012 06:55:59 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  LiuZhengzhen (), liuzhengzhen#gmail#com
 *   Organization:  
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <term.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <curses.h>
#include <stdio.h>
#include <termio.h>

static int COLUMN;
static int ROW;

int see_more(int precent)
{
	int c;
	fprintf(stdout, "\033[7m--more--(%d%)\033[m", precent);
	while ((c = getchar()) != EOF) {
		if (c == 'q')
			return 0;
		if (c == ' ')
			return ROW;
		if (c == '\n')
			return 1;
	}
	return 0;
}
int do_more(char *pathname)
{
	FILE *stream;
	char err[100];
	int nrow, ncolumn;
	char content[COLUMN];
	struct termios oldstat, newstat;
	int line = 0; //count the rows, when equals to ROW, pause outputting
	long long byte_count = 0;
	long long file_size;
	int replay;
	char *cursor;
	char *esc_sequence;
	struct stat buffer;
	stat(pathname, &buffer);
	file_size = (long long)buffer.st_size;

	stream = fopen(pathname, "r");
	if (!stream) {
		sprintf(err, "Can not open file %s", pathname);
		perror(err);
		exit(1);
	}

	tcgetattr(0, &oldstat);
	newstat = oldstat;
	newstat.c_lflag &= ~ICANON;
	newstat.c_lflag &= ~ECHO;
	newstat.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &newstat);

	setupterm(NULL, fileno(stdout), (int *)0);
	cursor = tigetstr("cup");
	ROW = tigetnum("lines") - 1;
	COLUMN = tigetnum("cols");
	esc_sequence = tparm(cursor, (ROW), 0);

	while (fgets(content, COLUMN, stream)) {
		byte_count += strlen(content);
		if (line == ROW) {
			replay = see_more(((byte_count * 100)/file_size));
			putp(esc_sequence);
			fputs("                   ", stdout);
			putp(esc_sequence);
			if (replay == 0)
				break;
			line -= replay;
		}

		if (fputs(content, stdout) == EOF) {
			perror("fputs error");
			exit(2);
		}
		line++;
	}

	tcsetattr(0, TCSANOW, &oldstat);
	fclose(stream);
}

int main(int argc, char *argv[]) 
{
	while (--argc) {
		do_more(*++argv);
	}
	
	return 0;
}
