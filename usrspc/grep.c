#include <string.h>
#include "ucode.c"

// prints line if pattern is found.
void findpattern(char* line, char* pattern)
{
	char tmp[128];
	char *tokens[32];
	char *tok;
	int i, j;
	
	strcpy(tmp, line);

	// tokenize line
	i = 0;
	tok = strtok(line, " ");
	while(tok)
	{
		tokens[i] = tok;
		i++;
		tok = strtok(NULL, " ");
	}

	// check tokens for match
	j = 0;
	while(j < i)
	{
		// If match found print line
		if(!strcmp(tokens[j], pattern))
			printf("%s\n", tmp);
		j++;
	}
}

int main(int argc, char *argv[ ])
{
	int file, count, i, j;
	char buff[17];
	char line[128];

	if(argc > 1)
		file = open(argv[2], O_RDONLY);

	j = 0;
	count = read(file, buff, 16);
	buff[count] = '\0';
	while(count)
	{
		i = 0;
		while(i < count)
		{
			if(buff[i] == '\n')
			{
				line[j] = '\0';
				findpattern(line, argv[1]);
				j = 0;
				i++;
				
			}
			else
			{
				line[j] = buff[i];
				j++;
				i++;
			}
		}
		
		count = read(file, buff, count);
		buff[count] = '\0';
	}

	close(file);
	return 0;
}

