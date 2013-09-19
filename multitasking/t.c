/************ t.c file **********************************/
#define NULL      0

#define NPROC     9        
#define SSIZE  1024                /* kstack int size */

#define FREE      0                /* proc status     */
#define DEAD      1
#define READY     2 
#define SLEEP     3      

typedef struct proc
{
	struct proc *next;
	int  ksp;			/* saved sp; offset = 2 */
	int  pid;
	int  ppid;
	struct proc *parentPtr;
	int  status;		/* FREE|DEAD|READY|SLEEP, etc */
	int  event;
	int  exitValue;
	int  priority;
	int  kstack[SSIZE];	// kmode stack of task
	}PROC;

//#include "io.c"  /* <===== use YOUR OWN io.c with printf() ****/

PROC proc[NPROC], *running, *freeQueue, *readyQueue, *sleepQueue;

int  procSize = sizeof(PROC);

/****************************************************************
 Initialize the proc's as shown:
        running ---> proc[0] -> proc[1];

        proc[1] to proc[N-1] form a circular list:

        proc[1] --> proc[2] ... --> proc[NPROC-1] -->
          ^                                         |
          |<---------------------------------------<-

        Each proc's kstack contains:
        retPC, ax, bx, cx, dx, bp, si, di, flag;  all 2 bytes
*****************************************************************/

int body();  
int enqueue(PROC *p, PROC **queue);
PROC *dequeue(PROC **queue);
int wait(int *status);


int initialize()
{
	int i, j;
	PROC *p;

	readyQueue = NULL;
	sleepQueue = NULL;

	for(i = 0; i < NPROC; i++)
	{
		p = &proc[i];
		p->next = &proc[i+1];
		p->pid = i;
		p->ppid = -1;
		p->parentPtr = NULL;
		p->status = READY;
    
		if(i)
		{     // initialize kstack[ ] of proc[1] to proc[N-1]
			for(j=1; j<10; j++)
				p->kstack[SSIZE - j] = 0;          // all saved registers = 0

			p->kstack[SSIZE-1] = (int)body;          // called tswitch() from body
			p->ksp = &(p->kstack[SSIZE-9]);        // ksp -> kstack top
			p->status = FREE;
		}
	}
	p->next = NULL;

	running = &proc[0];
	running->priority = 0;
	running->status = READY;

	freeQueue = &proc[1];

	if(!enqueue(running, &readyQueue))
	{
		printf("Epic fail adding to ready\n");
		return -1;
	}

	printf("initialization complete\n");
}

char *gasp[NPROC]=
{
	"Oh! You are killing me .......",
	"Oh! I am dying ...............", 
	"Oh! I am a goner .............", 
	"Bye! Bye! Crule World...............",      
};

int kexit()
{
	int i, parent;

	// P1 cannot die
	if(running->pid == 1)
	{
		printf("You cannot kill me I am P1!\n");
		return 1;
	}

	printf("\n*****************************************\n"); 
	printf("Task %d %s\n", running->pid,gasp[(running->pid) % 4]);
	printf("*****************************************\n");
	running->status = DEAD;

	parent = (int)running->parentPtr;

	// Give children to P1
	for(i = 0; i < NPROC; i++)
	{
		if(proc[i].ppid == running->pid)
		{
			proc[i].ppid = 1;
		}
	}
	// Wake Parent
	wakeup(parent);
	tswitch();   /* journey of no return */        
}

int ps()
{
	PROC *p;

	printf("running = %d\n", running->pid);

	p = readyQueue;
	printf("readyProcs = ");
	printQueue(readyQueue);
	printf("\n");
	printf("sleepingProcs = ");
	printQueue(sleepQueue);
	printf("\n");
}

int body()
{
	char c;
	int  s;
	
	while(1)
	{
		ps();
		printf("I am Proc %d in body()\n", running->pid);
		printf("Input a char : [s|q|f|w|l|u]\n");
		c=getc();
		switch(c)
		{
			case 's': tswitch(); break;
			case 'q': kexit();   break;
			case 'f': kfork();   break;
			case 'w': wait(&s);  break;
			case 'l': printf("Enter event: ");
					  c = getc();
					  putc(c);
					  printf("\n");
					  s = c - '0';
					  sleep(s);  break;
			case 'u': printf("Enter event: ");
					  c = getc();
					  putc(c);
					  printf("\n");
					  s = c - '0';
					  wakeup(s); break;
			default :            break;  
		}
	}
}


main()
{
	printf("\nWelcome to the 460 Multitasking System\n");
	initialize();
	printf("P0 fork P1\n");
	kfork();
	printf("P0 switch to P1\n");
	tswitch();
	printf("P0 resumes: all dead, happy ending\n");
}


int scheduler()
{
	PROC *p;
	
	if (running->status == READY)
		enqueue(running, &readyQueue);
	
	running = dequeue(&readyQueue);

	printf("\n-----------------------------\n");
	printf("next running proc = %d\n", running->pid);
	printf("-----------------------------\n");
}

PROC *getproc()
{
	return dequeue(&freeQueue);
}

int kfork()
{
	int i;
	PROC *p = getproc();

	if(!p)
	{
		printf("No more free processes.\n");
		return -1;
	}
	printf("%d", p->pid);
	
	p->ppid = running->pid;
	p->parentPtr = running;

	// setup kstack[]
	for (i = 1; i < 10; i++)
		p->kstack[SSIZE - i] = 0;          // all saved registers = 0

	p->kstack[SSIZE - 1] = (int)body;          // called tswitch() from body
	p->ksp = &(p->kstack[SSIZE - 9]);        // ksp -> kstack top

	p->status = READY;
	p->priority = 2;

	if(!enqueue(p, &readyQueue))
		return -1;

	return p->pid;
}

int enqueue(PROC *p, PROC **queue)
{
	PROC *cur;

	if(!*queue)
	{
		*queue = p;
		(*queue)->next = NULL;
		return 1;	
	}

	if((*queue)->priority < p->priority)
	{
		p->next = *queue;
		*queue = p;
		return 1;
	}

	cur = *queue;
	while(cur)
	{
		if(cur->priority >= p->priority)
		{
			if(!cur->next)
			{
				cur->next = p;
				p->next = NULL;
				return 1;
			}

			if(cur->next->priority < p->priority)
			{
				p->next = cur->next;
				cur->next = p;
				return 1;
			}
		}
		cur = cur->next;
	}

	printf("failed to add entry\n");
	return 0;
}

PROC *dequeue(PROC **queue)
{
	PROC *ret = *queue;
	if(*queue)
	{
		*queue = (*queue)->next;
		return ret;
	}
	return NULL;
}

int printQueue(PROC *queue)
{
	while(queue)
	{
		printf("%d -> ", queue->pid);
		queue = queue->next;
	}
}

int wait(int *status)
{
	int i;
	int c;
	// Find children
	for(i = 0, c = 0; i < NPROC; i++)
	{
		if(proc[i].ppid == running->pid && proc[i].status != FREE)
			c++;

		if(proc[i].status == DEAD && proc[i].ppid == running->pid)
		{
			*status = proc[i].exitValue;
			proc[i].status = FREE;
			return proc[i].pid;
		}
	}
	if(c > 0)
		sleep(running);
}

int sleep(int event)
{
	running->event = event;
	running->status = SLEEP;
	if(!enqueue(running, &sleepQueue))
		return -1;
	tswitch();	
}

int wakeup(int event)
{
	PROC **cur, *tar;
	cur = &sleepQueue;

	while(*cur)
	{
		if((*cur)->status == SLEEP && (*cur)->event == event)
		{
			(*cur)->status = READY;
			tar = *cur;
			*cur = (*cur)->next;
			if(!enqueue(tar, &readyQueue))
				return -1;
		}
		else
			cur = &(*cur)->next;

	}
}
