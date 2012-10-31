#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

ino_t get_inode_num(const char *path)
{
		struct stat buf;
		if (lstat(path, &buf))
			return -1;
		return buf.st_ino;
}

int main(void)
{
	DIR *dir;
	struct dirent *dir_item;
	ino_t father, child;
	char *path;

	child = get_inode_num(".");
	father = get_inode_num("..");

	while (child != father) {
			if (!(dir = opendir("..")))
					return -1;
			while (dir_item = readdir(dir)) {
				if (child == dir_item->d_ino) {
						*path = (char *)malloc(strlen(dir_item->d_name));
						break;
				}
			}
			closedir(dir);
		chdir("../");
		child = father;
		father = get_inode_num("..");
	} 
	return 0;
}
