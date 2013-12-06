#include <string.h>
#include "ucode.c"

int main(int argc, char *argv[ ])
{
	int infile, outfile, count;
	char buff[16];

	if(argc > 2)
	{
		infile = open(argv[1], READ);
		outfile = open(argv[2], WRITE);

		if(infile == -1)
		{
			printf("%s doesnt exisit.\n", argv[1]);
			return 1;
		}		
		if(outfile == -1)
		{
			creat(argv[2]);
			outfile = open(argv[2], WRITE);
		}
	}
	else
	{
		printf("Please specify files to copy.\n");
		return 1;
	}

	count = read(infile, buff, 16);
	while(count)
	{
		write(outfile, buff, count);
		count = read(infile, buff, 16);
	}
	close(infile);
	close(outfile);
}
