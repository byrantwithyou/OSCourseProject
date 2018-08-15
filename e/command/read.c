#include "stdio.h"
int main(int argc, char * argv[])
{/*filename : argv[1]*/
	int fd = open(argv[1], O_RDWR);
	int n = read(fd, bufr, rd_bytes);
    printl("%d bytes read: %s\n", n, bufr);
	close(fd);
    


	return 0;
}