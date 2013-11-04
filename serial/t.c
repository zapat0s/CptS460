// Written by Joshua Clark

#include "type.h"

/********* in type.h ***********
typedef unsigned char   u8;
typedef unsigned short u16;
typedef unsigned long  u32;

#define NULL     0
#define NPROC    9
#define SSIZE 1024

//******* PROC status ********
#define FREE     0
#define READY    1
#define RUNNING  2
#define STOPPED  3
#define SLEEP    4
#define ZOMBIE   5

typedef struct proc{
    struct proc *next;
    int    *ksp;

    int    uss, usp;           // at offsets 4, 6 
    int    pid;                // add pid for identify the proc
    int    status;             // status = FREE|READY|RUNNING|SLEEP|ZOMBIE    
    int    ppid;               // parent pid
    struct proc *parent;
    int    priority;
    int    event;
    int    exitCode;

    char   name[32];
    int    kstack[SSIZE];      // per proc stack area
}PROC;
*******************************/

PROC proc[NPROC], *running, *freeList, *readyQueue, *sleepList;
PIPE pipe[NPIPE];
OFTE oft[NOFTES];

int inkmode = 1;

#include "pv.c"
#include "kbd.c"
#include "serial.c"
#include "timer.c"

int procSize = sizeof(PROC);
int nproc = 0;

int body();
char *pname[]={"P0", "P1", "P2", "P3",  "P4", "P5","P6", "P7", "P8" };

int goUmode();

/**************************************************
  bio.o, queue.o loader.o are in mtxlib
**************************************************/
/* #include "bio.c" */
/* #include "queue.c" */
/* #include "loader.c" */

/**********
#include "wait.c"
#include "kernel.c"
#include "int.c"
************/

int do_tswitch()
{
	tswitch();
}

int do_kfork()
{
	kfork("/bin/u1");
}

int do_fork()
{
	PROC *p;
	int  i, offset;
	u16  segment;

	/*** get a PROC for child process: ***/
	if ( (p = get_proc(&freeList)) == 0){
		printf("no more proc\n");
		return -1;
	}

	/* initialize the new proc and its stack */
	p->status = READY;
	p->ppid = running->pid;
	p->parent = running;
	p->priority  = 1;                 // all of the same priority 1
	
	for(i =1; i < 10; i++)
		p->kstack[SSIZE - i] = 0;
	p->kstack[SSIZE - 1] = (u16)goUmode;
	p->ksp = &(p->kstack[SSIZE - 9]);

	// copy file descriptors	
	for(i = 0; i < NFD; i++)
	{
		p->fd[i] = p->parent->fd[i];
		if(p->fd[i])
		{
			p->fd[i]->refCount++;
			if(p->fd[i]->mode == READ_PIPE)
			{
				p->fd[i]->pipePtr->nreader++;
			}
			if(p->fd[i]->mode == WRITE_PIPE)
			{
				p->fd[i]->pipePtr->nwriter++;
			}
		}
	}
		
	enqueue(&readyQueue, p);
	
	// Setup Userspace
	segment = (p->pid + 1)*0x1000;
	copy_image(segment);

	p->usp = running->usp;
	p->uss = segment;

	put_word(segment, segment, p->usp);
	put_word(segment, segment, p->usp + 2);
	put_word(segment, segment, p->usp + 20);

	put_word(0, segment, p->usp + 16); 

	printf("Proc%d forked a child %d segment=%x\n", running->pid,p->pid,segment);
	return(p->pid);

}

int do_exec(int fptr)
{
	int i, offset;
	char fname[64];

	for(i = 0; i < 64; i++)
	{
		fname[i] = get_byte(running->uss, fptr + i);
		if(fname[i] == 0)
			break;
	}

	printf("loading %s into segment %x\n", fname, running->uss);

	load(fname, running->uss);

	offset = 0x1000;
	for(i = 1; i < 11; i++)
		put_word(0, running->uss, 0x1000 - (i * 2) ); 
	
	put_word(0x0200, running->uss, 0x1000 - 2);
	put_word(running->uss, running->uss, 0x1000 - 4);
	put_word(running->uss, running->uss, 0x1000 - 22);
	put_word(running->uss, running->uss, 0x1000 - 24);
	running->usp = 0x1000 -24;
}

int do_wait(int offset)
{
	int i, c, ret;

	//printf("%d", offset);

	// Find children
	for(i = 0, c = 0; i < NPROC; i++)
	{
		if(proc[i].ppid == running->pid &&
				(proc[i].status == READY || proc[i].status == RUNNING || proc[i].status == STOPPED))
			c++;
	}
	// Sleep if children found
	if(c > 0)
		sleep(running);

	// Wake dead processes
	for(i = 0; i < NPROC; i++)
	{
		if(proc[i].status == ZOMBIE && proc[i].ppid == running->pid)
		{
			put_word(proc[i].exitCode, running->uss, offset); //	return to usermode
			proc[i].status = FREE;
			ret = proc[i].pid;
			enqueue(&freeList, &proc[i]);
		}
	}

	return ret;
}

int do_exit(int val)
{
	kexit(val);	
}


int do_ps()
{
	int i;
	printf("Process Status\nPID STATUS   NAME\n");
	for(i = 0; i < NPROC; i++)
	{
		printf("%d  ", proc[i].pid);
		switch(proc[i].status)
		{
			case FREE:
				printf("FREE     ");
				break;
			case READY:
				printf("READY    ");
				break;
			case RUNNING:
				printf("RUNNING  ");
				break;
			case STOPPED:
				printf("STOPPED  ");
				break;
			case SLEEP:
				printf("SLEEPING ");
				break;
			case ZOMBIE:
				printf("ZOMBIE   ");
				break;
			default:
				printf("UNKNOWN  ");
				break;
		}
		printf("%s\n", proc[i].name);
	}
}

int kmode()
{
	body();
}

int chname(int offset)
{
	int i;

	for(i = 0; i < 64; i++)
	{
		running->name[i] = get_byte(running->uss, offset + i);
		if(running->name[i] == 0)
			break;
	}
	return 1;
}

int do_pipe(int loc)
{
	int fds[2];
	int ret;

	ret = kpipe(fds);

	put_word(fds[0], running->uss, loc);
	put_word(fds[1], running->uss, loc + 2);

	return ret;
}

int do_read(int fd, int loc, int bytes)
{
	char buf[128]; 
	int i, ret;

	ret = readPipe(fd, buf, bytes);

	for(i = 0; i < bytes; i++)
	{
		put_byte(buf[i], running->uss, loc + i);
	}

	return ret;
}

int do_write(int fd, int loc, int bytes)
{
	char buf[128];
	int i;

	for(i = 0; i < bytes; i++)
	{
		buf[i] = get_byte(running->uss, loc + i);
	}

	return writePipe(fd, buf, bytes);
}

int do_close(int fd)
{
	return closePipe(fd);
}

int init()
{
	PROC *p;
	int i, j;
	printf("init ....");
	for (i = 0; i < NPROC; i++)
	{   // initialize all procs
		p = &proc[i];
		p->pid = i;
		p->status = FREE;
		p->priority = 0;  
		strcpy(proc[i].name, pname[i]);

		p->next = &proc[i+1];

		for(j = 0; j < NFD; j++)
		{
			p->fd[j] = NULL;
		}
	}
	freeList = &proc[0];      // all procs are in freeList
	proc[NPROC - 1].next = 0;

	readyQueue = sleepList = 0;

	// initialize all pipes
	for(i = 0; i < NPIPE; i++)
	{
		pipe[i].head = 0;
		pipe[i].tail = 0;
		pipe[i].data = 0;
		pipe[i].room = PSIZE;
		pipe[i].nreader = 0;
		pipe[i].nwriter = 0;
		pipe[i].busy = 0;
	}

	/**** create P0 as running ******/
	p = get_proc(&freeList);
	p->status = READY;
	p->ppid   = 0;
	p->parent = p;
	running = p;
	nproc = 1;
	printf("done\n");
}

int int80h();


u16 set_vec(u16 vector, u16 handler)
{
	// put_word(word, segment, offset) in mtxlib
	put_word(handler, 0, vector<<2);
	put_word(0x1000,  0, (vector<<2) + 2);
}
            
main()
{
	printf("MTX starts in main()\n");
	init();      // initialize and create P0 as running
	
	kfork("/bin/u1");     // P0 kfork() P1
	
	set_vec(80, (u16)int80h);
	
	// Install kbd interupt handler, initilize kbd driver
	set_vec(9, (u16)kbhandler);
	kbinit();

	while(1)
	{
		printf("P0 running\n");
		while(!readyQueue);
		printf("P0 switch process\n");
		tswitch();         // P0 switch to run P1
	}
}

int scheduler()
{
	if (running->status == READY)
		enqueue(&readyQueue, running);
	running = dequeue(&readyQueue);
}

int body()
{
	char c;
	printf("proc %d resumes to body()\n", running->pid);
	while(1)
	{
		printf("-----------------------------------------\n");
		printList("freelist  ", freeList);
		printList("readyQueue", readyQueue);
		printList("sleepList ", sleepList);
		printf("-----------------------------------------\n");

		printf("proc %d running: parent = %d  enter a char [s|f|w|q|u] : ", 
		running->pid, running->parent->pid);

		c = getc(); printf("%c\n", c);
		switch(c)
		{
			case 's' : do_tswitch();   break;
			case 'f' : do_kfork();     break;
			case 'w' : do_wait(NULL);      break;
			case 'q' : do_exit(NULL);      break;

			case 'u' : goUmode();      break;
		}
	}
}
    
int kfork(char *filename)
{
	PROC *p;
	int  i, offset;
	u16  segment;

	/*** get a PROC for child process: ***/
	if ( (p = get_proc(&freeList)) == 0){
		printf("no more proc\n");
		return -1;
	}

	/* initialize the new proc and its stack */
	p->status = READY;
	p->ppid = running->pid;
	p->parent = running;
	p->priority  = 1;                 // all of the same priority 1

	/******* write C code to to do THIS YOURSELF ********************
	Initialize p's kstack AS IF it had called tswitch() 
	from the entry address of body():

	HI   -1  -2    -3  -4   -5   -6   -7   -8    -9                LOW
	-------------------------------------------------------------
	|body| ax | bx | cx | dx | bp | si | di |flag|
	------------------------------------------------------------
                                                ^
                                    PROC.ksp ---|

	******************************************************************/
	for(i =1; i < 10; i++)
		p->kstack[SSIZE - i] = 0;
	p->kstack[SSIZE - 1] = (u16)body;
	p->ksp = &(p->kstack[SSIZE - 9]);

	// copy file descriptors	
	for(i = 0; i < NFD; i++)
	{
		p->fd[i] = p->parent->fd[i];
		if(p->fd[i])
		{
			p->fd[i]->refCount++;
			if(p->fd[i]->mode == READ_PIPE)
			{
				p->fd[i]->pipePtr->nreader++;
			}
			if(p->fd[i]->mode == WRITE_PIPE)
			{
				p->fd[i]->pipePtr->nwriter++;
			}
		}
	}

	enqueue(&readyQueue, p);

	// make Umode image by loading /bin/u1 into segment
	segment = (p->pid + 1)*0x1000;
	load(filename, segment);

	/*************** WRITE C CODE TO DO THESE ******************
	Initialize new proc's ustak AS IF it had done INT 80
	from virtual address 0:

    HI  -1  -2  -3  -4  -5  -6  -7  -8  -9 -10 -11 -12
       flag uCS uPC uax ubx ucx udx ubp usi udi ues uds
     0x0200 seg  0   0   0   0   0   0   0   0  seg seg
                                                     ^
    PROC.uss = segment;           PROC.usp ----------|
 
	***********************************************************/
	offset = 0x1000;
	for(i = 1; i < 11; i++)
		put_word(0, segment, 0x1000 - (i * 2) ); 
	
	put_word(0x0200, segment, 0x1000 - 2);
	put_word(segment, segment, 0x1000 - 4);
	put_word(segment, segment, 0x1000 - 22);
	put_word(segment, segment, 0x1000 - 24);
	p->usp = 0x1000 -24;
	p->uss = segment;

	printf("Proc%d forked a child %d segment=%x\n", running->pid,p->pid,segment);
	return(p->pid);
}

int kpipe(int pd[2])
{
	int fd1, fd2, of1, of2, p;
	// find empty fd entries
	for(fd1 = 0; fd1 < NFD; fd1++)
	{
		if(!running->fd[fd1])
			break;
	}
	if(running->fd[fd1])
		return -1;

	for(fd2 = fd1 + 1; fd2 < NFD; fd2++)
	{
		if(!running->fd[fd2])
			break;
	}

	if(fd2 >= NFD)
		return -1;

	if(running->fd[fd2])
		return -1;

	// find empty oft entries
	for(of1 = 0; of1 < NOFTES; of1++)
	{
		if(oft[of1].refCount == 0)
			break;
	}

	if(oft[of1].refCount != 0)
		return -1;

	for(of2 = of1 + 1; of2 < NOFTES; of2++)
	{
		if(oft[of2].refCount == 0)
			break;
	}
	
	if(of2 >= NFD)
		return -1;

	if(oft[of2].refCount != 0)
		return -1;

	// find empty pipe entry
	for(p = 0; p < NPIPE; p++)
	{
		if(pipe[p].nreader == 0 && pipe[p].nwriter == 0)
			break;
	}

	if(pipe[p].nreader != 0 || pipe[p].nwriter != 0)
		return -1;

	// create pipe entry
	pipe[p].head = 0;
	pipe[p].tail = 0;
	pipe[p].data = 0;
	pipe[p].room = PSIZE;
	pipe[p].busy = false;
	pipe[p].nreader++;
	pipe[p].nwriter++;

	// create oft entries
	oft[of1].mode = READ_PIPE;
	oft[of1].refCount++;
	oft[of1].pipePtr = &(pipe[p]);

	oft[of2].mode = WRITE_PIPE;
	oft[of2].refCount++;
	oft[of2].pipePtr = &(pipe[p]);

	// create fd entries
	running->fd[fd1] = &(oft[of1]);
	running->fd[fd2] = &(oft[of2]);

	// populate pd array
	pd[0] = fd1;
	pd[1] = fd2;

	return true;
}

int closePipe(int fd)
{
	OFTE *op; PIPE *pp;

	printf("proc %d close_pipe: fd=%d\n", running->pid, fd);
	
	op = running->fd[fd];

	if(running->fd[fd] == NULL)				// return if fd already empty
		return 1;

	running->fd[fd] = NULL;                 // clear fd[fd] entry 

	if (op->mode == READ_PIPE)
	{
		pp = op->pipePtr;
		pp->nreader--;                   // dec n reader by 1

		if (--op->refCount == 0)
		{        // last reader
			if (pp->nwriter <= 0)
			{         // no more writers
				pp->busy = 0;             // free the pipe   
				return 1;
			}
		}
		wakeup(&pp->room); 
		return 1;
	}

	if (op->mode == WRITE_PIPE)
	{
		pp = op->pipePtr;
		pp->nwriter--;                   // dec nwriter by 1

		if (--op->refCount == 0)
		{        // last writer 
			if (pp->nreader <= 0)
			{         // no more readers 
				pp->busy = 0;              // free pipe also 
				return 1;
			}
		}
		wakeup(&pp->data);
		return 1;
	}
}

int readPipe(int fd, char *buf, int n)
{
	u16 loc, count;
	PIPE *pipePtr;

	showPipe(fd);

	// does fd exist
	if(!running->fd[fd])
		return -1;

	// is fd readable?
	if(running->fd[fd]->mode != READ_PIPE)
		return -1;

	// read data
	pipePtr = running->fd[fd]->pipePtr;
	loc = 0;

	for(count = 0; count < n; count++)
	{
		printf("%d bytes read\n", count);
		while(pipePtr->data <= 0)
		{
			printf("i should sleep here");
			wakeup(&(pipePtr->room));
			sleep(&(pipePtr->data));
		}

		if(pipePtr->tail >= PSIZE)
			pipePtr->tail = 0;
		buf[loc] = pipePtr->buf[pipePtr->tail];
		pipePtr->data--;
		pipePtr->tail++;
		pipePtr->room++;
		loc++;

		if(pipePtr->nwriter <= 0 && pipePtr->data <= 0)
			break;
	}

	wakeup(&(pipePtr->room));

	return count;
}

int writePipe(int fd, char *buf, int n)
{
	u16 loc, count;
	PIPE *pipePtr;

	// does fd exist
	if(!running->fd[fd])
		return -1;

	// is fd writable?
	if(running->fd[fd]->mode != WRITE_PIPE)
		return -1;

	// write data
	pipePtr = running->fd[fd]->pipePtr;
	loc = 0;
	count = 0;
	for(count = 0; count < n; count++)
	{
		while(pipePtr->room <= 0)
		{
			wakeup(&(pipePtr->data));
			sleep(&(pipePtr->room));
		}

		if(pipePtr->nreader <= 0 )
			break;
		
		if(pipePtr->head >= PSIZE)
			pipePtr->head = 0;
		
		pipePtr->buf[pipePtr->head] = buf[loc];
		pipePtr->room--;
		pipePtr->data++;
		pipePtr->head++;
		loc++;

	}

	showPipe(fd);

	wakeup(&(pipePtr->data));

	return count;
}

int showPipe(int fd)
{
	int i, j;
	PIPE *pipe = running->fd[fd]->pipePtr;
	printf("-------------- PIPE DATA --------------\n");     
	printf("readers: %d\t writers: %d\n", pipe->nreader, pipe->nwriter);
	printf("data: %d\t room: %d\n", pipe->data, pipe->room);
	printf("-------------PIPE CONTENTS ------------\n");
	for(i = 0; i < PSIZE; i++)
	{
		putc(pipe->buf[i]);
	}
	printf("\n-------------------------------------\n");
}

int printfd()
{
	int i;
	printf("File Descriptor Status\nFID MODE       REF COUNT\n");
	for(i = 0; i < NFD; i++)
	{
		printf("%d  ", i);
		switch(running->fd[i]->mode)
		{
			case READ_PIPE:
				printf("READ PIPE  ");
				break;
			case WRITE_PIPE:
				printf("WRITE PIPE ");
				break;	
			default:
				printf("UNKNOWN    ");
				break;
		}
		printf("%d\n", running->fd[i]->refCount);
	}

}

int sleep(int event)
{
	running->event = event;
	running->status = SLEEP;
	enqueue(&sleepList, running);
	tswitch();
	return 1;
}

int wakeup(int event)
{
	PROC **cur, *tar;
	cur = &sleepList;

	while(*cur)
	{
		if((*cur)->status == SLEEP && (*cur)->event == event)
		{
			printf("waking up P%d.\n", (*cur)->pid);
			(*cur)->status = READY;
			tar = *cur;
			*cur = (*cur)->next;
			enqueue(&readyQueue, tar);
		}
		else
			cur = &(*cur)->next;
	}
	return 1;
}


int kexit(int val)
{
	int i, parent;
	//P1 cannot die
	if(running->pid == 1)
	{
		printf("You cannot kill me I am P1!i\n");
		return -1;
	}

	printf("P%d exiting with code %d\n", running->pid, val);
	running->status = ZOMBIE;
	running->exitCode = val;

	for(i = 0; i < NPROC; i++)
	{
		if(proc[i].ppid == running->pid)
		{
			proc[i].ppid = 1;
		}
	}

	for(i = 0; i < NFD; i++)
	{
		closePipe(i);
	}

	wakeup((int)running->parent);
	tswitch();
}
/*************************************************************************
  usp  1   2   3   4   5   6   7   8   9  10   11   12    13  14  15  16
----------------------------------------------------------------------------
 |uds|ues|udi|usi|ubp|udx|ucx|ubx|uax|upc|ucs|uflag|retPC| a | b | c | d |
----------------------------------------------------------------------------
***************************************************************************/
#define PA 13
#define PB 14
#define PC 15
#define PD 16
#define AX  8

/****************** syscall handler in C ***************************/
int kcinth()
{
	u16    segment, offset;
	int    a,b,c,d, r;
	segment = running->uss; 
	offset = running->usp;

	/** get syscall parameters from ustack **/
	a = get_word(segment, offset + 2*PA);
	b = get_word(segment, offset + 2*PB);
	c = get_word(segment, offset + 2*PC);
	d = get_word(segment, offset + 2*PD);

	// UNCOMMENT TO SEE syscalls into kernel
	// printf("proc%d syscall a=%d b=%d c=%d d=%d\n", running->pid, a,b,c,d);

	switch(a)
	{
		case 0 : r = running->pid;     break;
		case 1 : r = do_ps();          break;
		case 2 : r = chname(b);        break;
		case 3 : r = kmode();          break;
		case 4 : r = tswitch();        break;
		case 5 : r = do_wait(b);       break;
		case 6 : r = do_exit(b);       break;
		case 7 : r = do_fork();		   break;
		case 8 : r = do_exec(b);	   break;
        
		case 80: r = do_pipe(b); 	   break;
		case 81: r = do_read(b, c, d); break;
		case 82: r = do_write(b, c, d);break;
		case 83: r = do_close(b); 	   break;

		case 84: r = printfd(); 	   break;

		case 90: r = getc();           break;
		case 91: r = putc(b);          break;       
		case 99: do_exit(b);           break;
		default: printf("invalid syscall # : %d\n", a); 
	}
	put_word(r, segment, offset + 2*AX);
}

int copy_image(int child_segment)
{
	int i, word;

	for(i = 0; i < 0x1000; i += 2)
	{
		word = get_word(running->uss, i);
		put_word(word, child_segment, i);
	}
}
