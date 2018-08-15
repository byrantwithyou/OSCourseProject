#include "stdio.h"
int main(int argc, char * argv[])
{
    /* filename : argv[1] */
	if(unlink(argv[1])==0)
        printl("File removed\n");
	else
        printf("Deleted failed\n");
	return 0;
}