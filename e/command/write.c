#include "stdio.h"
int main(int argc, char * argv[])
{/*filename : argv[1]*/
	int fd = open(argv[1], O_RDWR);
	int n = write(fd, argv[1], strlen(argv[1]));
	close(fd);
	return 0;
}