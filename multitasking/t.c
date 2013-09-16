/************ t.c file **********************************/
#define NULL      0

#define NPROC     9        
#define SSIZE  1024                /* kstack int size */

#define DEAD      0                /* proc status     */
#define FREE      2 
#define READY     1      

typedef struct proc
{
	struct proc *next;
	int  ksp;			/* saved sp; offset = 2 */
	int  pid;
	int  ppid;
	struct proc *parentPtr;
	int  status;		/* READY|DEAD, etc */
	int  kstack[SSIZE];	// kmode stack of task
	int  priority;
}PROC;

//#include "io.c"  /* <===== use YOUR OWN io.c with printf() ****/

PROC proc[NPROC], *running, *free, *ready;

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


int initialize()
{
	int i, j;
	PROC *p;

	ready = 0;

	for(i = 0; i < NPROC; i++)
	{
		p = &proc[i];
		p->next = &proc[i+1];
		p->pid = i;
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

	free = &proc[1];

	if(!enqueue(running, &ready))
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

int grave()
{
	printf("\n*****************************************\n"); 
	printf("Task %d %s\n", running->pid,gasp[(running->pid) % 4]);
	printf("*****************************************\n");
	running->status = DEAD;

	tswitch();   /* journey of no return */        
}

int ps()
{
	PROC *p;

	printf("running = %d\n", running->pid);

	p = ready;
	printf("readyProcs = ");
	printQueue(ready);
	printf("\n");
}

int body()
{
	char c;
	
	while(1)
	{
		ps();
		printf("I am Proc %d in body()\n", running->pid);
		printf("Input a char : [s|q|f]\n");
		c=getc();
		switch(c)
		{
			case 's': tswitch(); break;
			case 'q': grave();   break;
			case 'f': kfork();   break;
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
		enqueue(running, &ready);
	
	running = dequeue(&ready);

	printf("\n-----------------------------\n");
	printf("next running proc = %d\n", running->pid);
	printf("-----------------------------\n");
}

PROC *getproc()
{
	return dequeue(&free);
}

int freeproc(PROC *p)
{
	p->ppid = 0;
	p->status = DEAD;
	p->priority = 0;
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

	// setup kstack[]
	for (i = 1; i < 10; i++)
		p->kstack[SSIZE - i] = 0;          // all saved registers = 0

	p->kstack[SSIZE - 1] = (int)body;          // called tswitch() from body
	p->ksp = &(p->kstack[SSIZE - 9]);        // ksp -> kstack top

	p->status = READY;
	p->priority = 2;

	if(!enqueue(p, &ready))
		return -1;

	return p->pid;
}

int enqueue(PROC *p, PROC **queue)
{
	PROC *cur;

	if(!*queue)
	{
		*queue = p;
		ready->next = NULL;
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
