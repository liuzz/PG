#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

void inum_to_name(ino_t, char *, int);
ino_t get_inode(char *);
void printpathto(ino_t this_inode)
{
		ino_t my_inode;
		char its_name[BUFSIZ];
		if (get_inode("..") != this_inode) {
				chdir("..");
				inum_to_name(this_inode, its_name, BUFSIZ);
				my_inode = get_inode(".");
				printpathto(my_inode);
				printf("/%s", its_name);
		}
}

void inum_to_name(ino_t inode_to_find, char *namebuf, int buflen)
{
		DIR		*dir_ptr;
		struct	dirent *direntp;
		dir_ptr = opendir(".");
		if (dir_ptr == NULL) {
				perror(".");
				exit(1);
		}
		while ((direntp = readdir(dir_ptr)) != NULL)
				if (direntp->d_ino == inode_to_find) {
						strncpy(namebuf, direntp->d_name, (size_t)buflen);
						namebuf[buflen - 1] = '\0'; //see the man page of strncpy
						closedir(dir_ptr);
						return;
				}
		fprintf(stderr, "error looking for inum %d\n", inode_to_find);
		exit(1);
}
ino_t get_inode(char *fname)
{
		struct stat info;
		if (stat(fname, &info) == -1) {
				fprintf(stderr, "Cannot stat ");
				perror(fname);
				exit(1);
		}
		return info.st_ino;
}
int main(void)
{
		printpathto( get_inode(".") );
		putchar('\n');
		return 0;
}
