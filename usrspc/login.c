#include <string.h>
#include "ucode.c"

struct user
{
	char username[32];
	char password[32];
	int uid;
	int gid;
	char info[16];
	char home[16];
	char shell[16];
};

struct user users[16];

// Returns index of value in list.
int validate(char *username, char *password)
{
	int i;

	for(i = 0; i < 16; i++)
	{
		if(!strcmp(users[i].username, username)
			&& !strcmp(users[i].password, password))
			return i;
	}
	return -1;
}

main(int argc, char *argv[ ])
{
	char *tty, *tok;
	char buff[256];
	char buff2[16];
	int loginfile, i, j;

	if(argc <= 1)
		printf("Please Specify device\n");

	tty = argv[1];

	// Free up stdin, stdout, and strerr
	close(0);
	close(1);
	close(2);

	// Set stdin, stdout, and stderr to tty
	open(tty, READ);
	open(tty, WRITE);
	open(tty, WRITE);

	settty(tty);

	// Ignore Ctrl-C
	signal(2, 1);

	// Open login file
	loginfile = open("/etc/passwd", READ);

	read(loginfile, buff, 256);

	// Close login file
	close(loginfile);

	// Process login file
	tok = strtok(buff, ":\n");
	j = 0;
	while(tok)
	{
		for(i = 0; i < 7; i++)
		{
			switch(i)
			{
				case 0: // username
					strcpy(users[j].username, tok);
					break;
				case 1: // password
					strcpy(users[j].password, tok);
					break;
				case 2: // uid
					users[j].uid = atoi(tok);
					break;
				case 3: // gid
					users[j].gid = atoi(tok);
					break;
				case 4: // info
					strcpy(users[j].info, tok);
					break;
				case 5: // Home directory
					strcpy(users[j].home, tok);
					break;
				case 6: // Shell
					strcpy(users[j].shell, tok);
					break;
			}
			tok = strtok(NULL, ":\n");
		}
		j++;
	}

	// Login Prompt
	while(1)
	{
		printf("login: ");
		// Read login
		gets(buff);

		printf("password: ");
		// Read password
		gets(buff2);

		// Validate Info
		i = validate(buff, buff2);

		if(i != -1)
		{
			printf("Welcome %s.\n", users[i].username);
			exec(users[i].shell);
		}
		else
			printf("Incorrect Username or Password.\n");
	}
		
}
