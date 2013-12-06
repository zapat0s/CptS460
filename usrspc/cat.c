#include <string.h>
#include "ucode.c"

int main(int argc, char *argv[ ])
{
	int file, count;
	char buff[17];

	if(argc > 1)
		file = open(argv[1]);
	else
		file = 0;

	count = read(file, buff, 16);
	while(count)
	{
		printf(buff);
		count = read(file, buff, count);
		buff[count] = '\0';
	}

	close(file);
	return 0;
}
