#include "ucode.c"
int color;

main()
{ 
	char name[64]; int pid, cmd;
  

	while(1)
	{
		pid = getpid();
		color = 0x0000B + pid;

		printf("==============================================\n");
		printf("\nIch bin Prozess %d in der U Weise: das laufen im Segment=%x\n", getpid(), getcs());
		show_menu();
		printf("Command ? ");
		gets(name);
		if(name[0] == 0)
			continue;

		cmd = find_cmd(name);
		switch(cmd)
		{
			case 0 : getpid();   break;
			case 1 : ps();       break;
			case 2 : chname();   break;
			case 3 : kmode();    break;
			case 4 : kswitch();  break;
			case 5 : wait();     break;
			case 6 : exit();     break;
			case 7 : fork();	 break;
			case 8 : exec();     break;

			default: invalid(name); break;
		}
	}
}



