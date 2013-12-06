// Created By Joshua Clark
#include <string.h>

int pid, child, status;
int stdin, stdout;

#include "ucode.c"

// Returns index of value in list.
int contains(int *list, int length, int val)
{
	int i;
	for(i = 0; i < length; i++)
	{
		if(list[i] == val)
			return i;
	}
	return -1;
}

// Create command and execute it.
int login(char* path)
{
	char command[128];
	strcpy(command, "login ");
	strcat(command, path);
	exec(command);
}

void main(int argc, char *argv[])
{
	int children[8];
	char childdevs[8][32];
	int devicefd, i;
	char buff[128];
	char *tok;

	// setup io
	stdin = open("/dev/tty0", O_RDONLY);
	stdout = open("/dev/tty0", O_WRONLY);
	
	// open list of devices
	devicefd = open("/etc/init", O_RDONLY);
	read(devicefd, buff, 128);

	// split list by line and start login processes
	tok = strtok(buff, "\n");

	i = 0;
	while(tok)
	{	
		child = fork();
		if(child == 0) // If child
			login(tok);

		children[i] = child;
		strcpy(childdevs[i], tok);

		tok = strtok(NULL, "\n");
		i++;
	}

	// Wait for children to die.
	while(1)
	{
		pid = wait(&status);
		i = contains(children, 8, pid);

		if(i != -1)
			login(childdevs[i]);
		else
			printf("INIT: buried an orphan child %d\n", pid);
	}

}

