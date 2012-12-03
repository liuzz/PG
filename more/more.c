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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <curses.h>
#include <stdio.h>
#include <termio.h>

//#define COLUMN 		160
//#define ROW 		27

int see_more(int ROW)
{
	int c;
	fprintf(stdout, "\033[7m --more-- \033[m");
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
	int ROW, COLUMN;
	char err[100];
	int nrow, ncolumn;
	char content[COLUMN];
	struct termios oldstat, newstat;
	int line = 0; //count the rows, when equals to ROW, pause outputting
	int replay;
	char *cursor;
	char *esc_sequence;
	struct stat buffer;
	stat(pathname, &buffer);


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
	ROW = tigetnum("lines");
	COLUMN = tigetnum("cols");
	esc_sequence = tparm(cursor, (ROW + 1), 0);

	while (fgets(content, COLUMN, stream)) {
		if (line == ROW) {
			replay = see_more(ROW);
			putp(esc_sequence);
			fputs("          ", stdout);
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
	printf("%ld\n", (long)buffer.st_size);
}

int main(int argc, char *argv[]) 
{
	while (--argc) {
		do_more(*++argv);
	}
	
	return 0;
}
