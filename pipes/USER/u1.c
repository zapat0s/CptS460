#include "ucode.c"
int color;
main()
{ 
  char name[64]; int pid, cmd;

  while(1){
    pid = getpid();

    printf("----------------------------------------------\n");
    printf("I am proc %d in U mode: running segment=%x\n",getpid(), getcs());
    show_menu();
    printf("> ");
    gets(name); 
    if (name[0]==0) 
        continue;

    cmd = find_cmd(name);
    switch(cmd){
           case 0 : getpid();   break;
           case 1 : ps();       break;
           case 2 : chname();   break;
           case 3 : kmode();    break;
           case 4 : kswitch();  break;
           case 5 : wait();     break;
           case 6 : exit();     break;
		   case 7 : fork();		break;
		   case 8 : exec();     break;
		   case 9 : pipe();     break;
		   case 10: read();     break;
		   case 11: write();    break;
		   case 12: close();    break;
		   case 13: printfd();  break;

           default: invalid(name); break;
    }
  }
}



