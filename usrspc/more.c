#include <string.h>
#include "ucode.c"

typedef int bool;
enum { false, true };

int end;

void printline(int fd)
{
	int count, rcount;
	char buff[1];
		 
	count = 0;
	printf("\n");
	do
	{
		rcount = read(fd, buff, 1);
		if(rcount == 0)
		{
			end = true;
			return;
		}
		if(buff[0] != '\n')
			printf("%c", buff[0]);
		else if (buff[0] == '\n')
		{
			return;
		}
		count++;
	}
	while(count < 81);
}

void printscreen(int fd)
{
	int count, rcount, x, y;
	char buff[1];

	y = 0;
	while(y < 25)
	{
		printline(fd);
		y++;
	}
}


int main(int argc, char *argv[ ])
{
	int file;
	char c;

	end = false;

	if(argc > 1)
		file = open(argv[1], O_RDONLY);
	else
		file = 0;
	
	if(file == -1)
		printf("Unable to open file");

	printscreen(file);
	while(true)
	{
		if(end)
			break;
		
		c = getc();
		
		if(c == '\r')
			printline(file);
		else if(c == ' ')
			printscreen(file);
		else if(c == 'q')
			break;
	}

	close(file);
	return 0;
}

