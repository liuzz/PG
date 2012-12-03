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

static int term_noecho(void)
{
	struct termios newstat;

	tcgetattr(0, &newstat);
	newstat.c_lflag &= ~ICANON;
	newstat.c_lflag &= ~ECHO;
	newstat.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &newstat);

	return 0;
}

static int termios_info(int flag)
{
	static struct termios oldstat;
	if (flag == 0) {
		tcgetattr(0, &oldstat);
	} else {
		tcsetattr(0, TCSANOW, &oldstat);
	}
	return 0;
}

static int get_col_row()
{
	setupterm(NULL, fileno(stdout), (int *)0); // /usr/share/terminfo/; infocmp
	ROW = tigetnum("lines") - 1;
	COLUMN = tigetnum("cols");

	return 0;
}

static int clean_prompt(void)
{
	static char *esc_sequence;
	if (esc_sequence == NULL) {
		char *cursor;
		cursor = tigetstr("cup"); // This is the magic curse which you can jump to the 
									//cursor wherever you want, oh sorry, just limit in the terminal.
		esc_sequence = tparm(cursor, (ROW), 0);
	}

	putp(esc_sequence);
	fputs("                   ", stdout);
	putp(esc_sequence);

	return 0;
}

static long long get_file_size(char *pathname)
{
	struct stat buffer;
	stat(pathname, &buffer);
	return (long long)buffer.st_size;
}

static int do_more(char *filename)
{
	FILE *stream;
	char err[100];
	char content[COLUMN];
	int line = 0; //count the rows, when equals to ROW, pause outputting
	long long byte_count = 0; //count for the percent
	long long file_size; 	  
	int replay;

	file_size = get_file_size(filename);

	stream = fopen(filename, "r");
	if (!stream) {
		sprintf(err, "Can not open file %s", filename);
		perror(err);
		exit(1);
	}

	termios_info(0); //get the original termios before setting.
	term_noecho(); 	//make the termial no echo and do not to enter "Enter"

	while (fgets(content, COLUMN, stream)) {
		byte_count += strlen(content);
		if (line == ROW) {
			replay = see_more(((byte_count * 100)/file_size));
			clean_prompt();
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

	termios_info(1); //recover the termios 
	fclose(stream);
}

int main(int argc, char *argv[]) 
{
	get_col_row(); //get the number of ROW and COLUMN for this special terminal,
					// by the way run setupterm() for clean_prompt().
	while (--argc) {
		do_more(*++argv);
	}
	
	return 0;
}
