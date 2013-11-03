// ucode.c file

char *cmd[]={"getpid", "ps", "chname", "kmode", "switch", "wait", "exit", "fork", "exec", "pipe", "read", "write", "close", "printfd", 0};

int show_menu()
{
   printf("***************** Menu ********************************\n");
   printf("*  ps  chname  kmode  switch  wait  exit  fork  exec  *\n*  pipe  read  write  close  printfd                  *\n");
   printf("*******************************************************\n");
}

int find_cmd(name) char *name;
{
   int i=0;   
   char *p=cmd[0];

   while (p){
         if (strcmp(p, name)==0)
            return i;
         i++;  
         p = cmd[i];
   } 
   return(-1);
}

int getpid()
{
   return syscall(0,0,0);
}

int ps()
{
   syscall(1, 0, 0);
}

int chname()
{
    char s[64];
    printf("input new name : ");
    gets(s);
    syscall(2, s, 0);
}

int kmode()
{
    //printf("kmode : syscall #3 to enter Kmode\n");
    //printf("proc %d going K mode ....\n", getpid());
        syscall(3, 0, 0);
    //printf("proc %d back from Kernel\n", getpid());
}    

int kswitch()
{
    //printf("proc %d enter Kernel to switch proc\n", getpid());
        syscall(4,0,0);
    //printf("proc %d back from Kernel\n", getpid());
}

int fork()
{
	int child;
	//printf("proc %d enter Kernel to switch proc\n", getpid());
	child = syscall(7, 0, 0);
	//printf("proc %d back from Kernel\n", getpid());
	printf("this is proc %d and %d was just forked\n", getpid(), child);
	return child;
}

int exec()
{
	char s[64];
	printf("input filename now: ");
	gets(s);
	printf("%d", &s);
	syscall(8, s, 0);	
}	

int wait()
{
    int child, exitValue;

	exitValue = 0;
    //printf("proc %d enter Kernel to wait for a child to die\n", getpid());
    child = syscall(5, &exitValue, 0);
    printf("proc %d back from wait, dead child=%d", getpid(), child);
    if (child>=0)
        printf("exitValue=%d", exitValue);
    printf("\n"); 
} 

int exit()
{
   int exitValue;
   printf("\nenter an exitValue (0-9) : ");
   exitValue=(getc()&0x7F) - '0';
   //printf("enter kenel to die with exitValue=%d\n", exitValue);
   _exit(exitValue);
}

int _exit(exitValue) int exitValue;
{
  return syscall(6,exitValue,0);
}

int pipe()
{
	int fd[2];
	int ret;
	ret = syscall(80, fd, 0, 0);
	printf("Pipe created on fds %d and %d\n", fd[0], fd[1]);
	return ret;
}

int read()
{
	char s[64];
	int fd, bytes, ret;
    printf("fd to read from : ");
    gets(s);
	fd = atoi(s);
	printf("\nbytes to read : ");
    gets(s);
	bytes = atoi(s);

	if(bytes > 64)
		bytes = 64;

	ret = syscall(81, fd, s, bytes);
	printf("%s\n", s);
}

int write()
{
	char s[64];
	int fd, bytes;
    printf("fd to write to : ");
    gets(s);
	fd = atoi(s);
	printf("\nenter string : ");
    gets(s);
	for(bytes = 0; bytes < 64; bytes++)
	{
		if(s[bytes] == 0)
			break;
	}

	return syscall(82, fd, s, bytes);
}

int close()
{
	char s[2];
	int fd;
    printf("fd to close : ");
    gets(s);
	fd = atoi(s);

	return syscall(83, fd, 0, 0);
}

int printfd()
{
	return syscall(84, 0, 0, 0);
}

int getc()
{
  return syscall(90,0,0) & 0x7F;
}

int putc(c) char c;
{
  return syscall(91,c,0,0);
}


int invalid(name) char *name;
{
    printf("Invalid command : %s\n", name);
}
