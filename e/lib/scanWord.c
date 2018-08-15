#include "stdio.h"

PUBLIC void scanWord(int *argc2, char *argv2[])
{
	char rdbuf[128];

	int r = read(0, rdbuf, 70);
	rdbuf[r] = 0;
	*argc2 = 0;
	char * p = rdbuf;
	char * s;
	int word = 0;
	char ch;
	do {
		ch = *p;
		if (*p != ' ' && *p != 0 && !word) {
			s = p;
			word = 1;
		}
		if ((*p == ' ' || *p == 0) && word) {
			word = 0;
			argv2[(*argc2)++] = s;
			*p = 0;
		}
		p++;
	} while(ch);
	argv2[*argc2] = 0;
	return;
}

