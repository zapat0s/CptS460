#include <string.h>
#include "ucode.c"

int main(int argc, char *argv[ ])
{
	int infile, outfile, count;
	char buff[16];

	if(argc > 2)
	{
		infile = open(argv[1], O_RDONLY);
		outfile = open(argv[2], O_WRONLY | O_CREAT);

		if(infile == -1)
		{
			printf("%s doesnt exisit.\n", argv[1]);
			return 1;
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
