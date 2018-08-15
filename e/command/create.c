#include "stdio.h"
int main(int argc, char * argv[])
{
	int i;
    /* filename : argv[1] */
	int fd = open(argv[1], O_CREAT);
	printf("Created file, it's fd: %d\n", fd);
	close(fd);
	return 0;
}