#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#define	BUFSIZE	4096
#define	FILEMODE	0644

int main(int argc, char *argv[])
{
		char *src = *++argv;
		char *des = *++argv;
		char buf[BUFSIZE];
		int srcfd, desfd, readnum, writenum;

		printf("src %s\n", src);
		printf("des %s\n", des);

		if ((srcfd = open(src, O_RDONLY)) == -1) {
				printf("open src error\n");
				return -1;
		}

		if ((desfd = creat(des, FILEMODE)) == -1) {
				printf("create des error\n");
				return -1;
		}

		while ((readnum = read(srcfd, buf, BUFSIZE)) > 0) {
				writenum = write(desfd, buf, readnum);
				if (writenum != readnum) {
						printf("write error\n");
						return -2;
				}
		}

		close(desfd);
		close(srcfd);

		return 0;
}
