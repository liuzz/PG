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
 *         Author:  Liu Zhengzhen (), liuzhengzhen9036#gmail#com
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

static int see_more(int percent, FILE *fp)
{
	int c;
	if (percent)
		fprintf(stdout, "\033[7m--more--(%d%)\033[m", percent);
	else
		fprintf(stdout, "\033[7m--more--\033[m");
	while ((c = getc(fp)) != EOF) {
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

	tcgetattr(1, &newstat);
	newstat.c_lflag &= ~ICANON;
	newstat.c_lflag &= ~ECHO;
	newstat.c_cc[VMIN] = 1;
	tcsetattr(1, TCSANOW, &newstat);

	return 0;
}

static int termios_info(int flag)
{
	static struct termios oldstat;
	if (flag == 0) {
		tcgetattr(1, &oldstat);
	} else {
		tcsetattr(1, TCSANOW, &oldstat);
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
		cursor = tigetstr("cup"); // This is the magic curses which you can jump to the 
									//cursor wherever you want, oh sorry, just limit in the terminal.
		esc_sequence = tparm(cursor, (ROW), 0);
	}

	putp(esc_sequence);
	fputs("                 ", stdout);
	putp(esc_sequence);

	return 0;
}

static long long get_file_size(char *pathname)
{
	struct stat buffer;
	stat(pathname, &buffer);
	return (long long)buffer.st_size;
}

static int do_more(long long file_size, FILE *stream)
{
	char content[COLUMN];
	int line = 0; //count the rows, when equals to ROW, pause outputting
	long long byte_count = 0; //count for the percent
	int replay;
	FILE *fp_tty;

	termios_info(0); //get the original termios before setting.
	term_noecho(); 	//make the termial no echo and not need to enter "Enter"

	fp_tty = fopen("/dev/tty", "r");
	if (!fp_tty)
		exit(1);
	while (fgets(content, COLUMN, stream)) {
		byte_count += strlen(content);
		if (line == ROW) {
			replay = see_more((file_size ? ((byte_count * 100)/file_size): 0), fp_tty);
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
	fclose(fp_tty);
}

int main(int argc, char *argv[]) 
{
	FILE *stream;
	char err[100];
	long long file_size;
	FILE *fp_tty;
	get_col_row(); //get the number of ROW and COLUMN for this special terminal,
					// by the way run setupterm() for clean_prompt().
	if (argc == 1) {
			do_more(0, stdin);
	} else {
		while (--argc) {
			stream = fopen(*++argv, "r");
			if (!stream) {
				sprintf(err, "Can not open file %s", *argv);
				perror(err);
				exit(1);
			}
			file_size = get_file_size(*argv);
			do_more(file_size, stream);
		}
	}
	
	return 0;
}
