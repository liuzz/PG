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
#include <stdio.h>

#define COLUMN 		160
#define ROW 		48

int see_more(void)
{
	int c;
	fseek(stdout, 20, SEEK_END);
	fprintf(stdout, "\033[7m more? \033[m");
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
	char content[COLUMN];
	int line = 0; //count the rows, when equals to ROW, pause outputting
	int replay;

	stream = fopen(pathname, "r");
	if (!stream) {
		sprintf(err, "Can not open file %s", pathname);
		perror(err);
		exit(1);
	}

	while (fgets(content, COLUMN, stream)) {
		if (line == ROW) {
			replay = see_more();
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

	fclose(stream);
}

int main(int argc, char *argv[]) 
{
	while (--argc) {
		do_more(*++argv);
	}
	
	return 0;
}
