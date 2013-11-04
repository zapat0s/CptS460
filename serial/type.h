typedef unsigned char   u8;
typedef unsigned short u16;
typedef unsigned long  u32;

#define NULL 0
#define true 1
#define false 0

#define NPROC    9
#define SSIZE 1024
#define NOFTES  20
#define NFD     10

#define PSIZE   10
#define NPIPE   10

#define READ_PIPE 4
#define WRITE_PIPE 5

/******* PROC status ********/
#define FREE     0
#define READY    1
#define RUNNING  2
#define STOPPED  3
#define SLEEP    4
#define ZOMBIE   5
#define BLOCK 	 6

typedef struct pipe
{
	char buf[PSIZE];
	int head, tail, data, room;
	int nreader, nwriter;
	int busy;
} PIPE;

typedef struct ofte
{
	int mode;
	int refCount;
	PIPE *pipePtr;
} OFTE;

typedef struct proc
{
	struct proc *next;
	
	// Registers
	int    *ksp;               // at offset 2
	int    uss, usp;           // at offsets 4,6

	// identity
	int    pid;                // add pid for identify the proc
	int    status;             // status = FREE|READY|RUNNING|SLEEP|ZOMBIE    
	int    ppid;               // parent pid
	struct proc *parent;

	// properties
	int    priority;
	int    event;
	int    exitCode;
	char   name[32];

	OFTE    *fd[NFD];
	int    kstack[SSIZE];      // per proc stack area
}PROC;

