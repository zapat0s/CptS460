#include "ucode.c"

main()
{ 
	char c;
	while(1)
	{
		printf("This is P1 in Umode segment=%x\n", getcs());
		printf("input a char : "); c=getc();
		printf("c=%c\n", c);
	}
}
