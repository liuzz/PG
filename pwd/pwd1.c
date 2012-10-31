#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>

int main(void)
{
	DIR *dir;
	struct dirent *dir_item;

	if (!(dir = opendir("..")))
			return -1;

	if (!(dir_item = readdir(dir)))
			return -1;

	printf("%s\n", dir_item->d_name);

	return 0;
}
