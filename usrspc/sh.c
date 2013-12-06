#include <string.h>
#include "ucode.c"

typedef int bool;
enum { false, true };

void runCommand(char *pipetokens[], int pipecount, int pos)
{
	char cmd[128];
	char *inputtokens[128], *tok;
	int tokencount = 0, pid, status, i, j, pipes[2];

	if(pipecount - pos > 0)
	{
		// Create pipe
		pipe(pipes);

		// Fork and wait for child to return
		pid = fork();

		if(!pid) // child
		{
			// setup pipes
			close(1);
			dup(pipes[1]);
			close(pipes[0]);

			runCommand(pipetokens, pipecount, ++pos);
		}
		else // parent
		{
			close(0);
			dup(pipes[0]);
			close(pipes[1]);
		}
	}

	tok = strtok(pipetokens[pipecount -pos], " ");	
	while(tokencount < 256 && tok)
	{
		inputtokens[tokencount] = tok;
		tokencount++;
		tok = strtok(NULL, " ");
	}
	pipetokens[pipecount] = 0;

	// Deal with IO redirection
	i = 0;
	while(inputtokens[i])
	{
		if(!strcmp(inputtokens[i], "<")
			|| !strcmp(inputtokens[i], ">")
			|| !strcmp(inputtokens[i], ">>"))
		{
			break;
		}
		i++;
	}

	if(inputtokens[i])
	{
		if(!strcmp(inputtokens[i], "<"))
		{
			inputtokens[i] = 0;
			close(0);
			open(inputtokens[i + 1], O_RDONLY);
		}
		else if(!strcmp(inputtokens[i], ">"))
		{
			inputtokens[i] = 0;
			close(1);
			open(inputtokens[i + 1], O_WRONLY | O_CREAT);
		}
		else if(!strcmp(inputtokens[i], ">>"))
		{
			inputtokens[i] = 0;
			close(1);
			open(inputtokens[i + 1], O_WRONLY | O_APPEND | O_CREAT);
		}
	}

	// Create command
	strcpy(cmd, inputtokens[0]);
	for(j = 1; j < i; j++)
	{
		strcat(cmd, " ");
		strcat(cmd, inputtokens[j]);
	}

	// Execute Command
	exec(cmd);
	
}

int main(int argc, char *argv[])
{
	while(true)
	{
		char input[512], cmd[128];
		char *inputtokens[128], *tok;
		int tokencount = 0, pid, status, i, j;
		char *pipetokens[64];
		int pipecount = 0;

		printf("# ");
		gets(input);
		
		if(input)
		{
			i = strlen(input);
			if(input[i - 1] == '\n')
				input[i - 1] = '\0';
		}

		// Empty String
		if(!strcmp(input, ""))
			continue;

		// Simple Commands
		if(!strcmp(input, "logout"))
			return;

		if(!strcmp(input, "pwd"))
		{
			getcwd(input);
			printf("%s\n", input);
			continue;	
		}

		strncpy(cmd, input, 2);
		cmd[2] = '\0';
		if(!strcmp(cmd, "cd"))
		{
			tok = &(input[3]);
			chdir(tok);
			continue;
		}

		// Find pipes
		tok = strtok(input, "|");	
		while(pipecount < 256 && tok)
		{
			pipetokens[pipecount] = tok;
			pipecount++;
			tok = strtok(NULL, "|");
		}
		pipetokens[pipecount] = 0;

		pid = fork();	
		if(pid)	// Parent
		{
			status = 0;
			pid = wait(&status);
		}
		else
			runCommand(pipetokens, pipecount, 1);
	}
}
