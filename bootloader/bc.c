/*******************************************************
*                  @bc.c file                          *
*******************************************************/
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned long  u32;

#include "ext2.h"
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;

#define BLK 1024
char kstr[16];
char buf1[BLK], buf2[BLK];

int prints(char *s)
{
	while(*s)
	{
		putc(*s++);
	}
}

int gets(char *s)
{
	char c;
	while ( (c = getc()) != '\r')
	{
		*s++ = c;
		putc(c);
	}
	*s = 0;
}

u16 getblk(u16 blk, char *buf)
{
	readfd( blk/18, ((blk*2)%36)/18, ((blk*2)%36)%18, buf);
}

u16 findentry(u16 block, char* name)
{
	char *c;
	DIR *dp;

	getblk(block, buf2);
	dp = (DIR *)buf2;         // buf2 contains DIR entries

	while((char *)dp < &buf2[BLK])
	{
		c = dp->name[dp->name_len];
		dp->name[dp->name_len] = 0;
		
		if(strcmp(dp->name, name) == 0)
		{
			return dp->inode;
		}

		dp = (DIR *)((char *)dp + dp->rec_len);
	}
}

INODE *getinode(u16 iblk, u16 inode)
{
	u16 block, idx;

	block = iblk + ((inode - 1) / 8); 
	idx = (inode - 1) % 8;

	getblk(block, buf1);
			
	return (INODE *)buf1 + idx;
}

int main()
{ 
	u16    i,iblk;
	char  *loc;
	u32   *l;
	GD    *gp;
	INODE *ip;

	prints("What image: ");
	gets(kstr);
	prints("\r\n");

	// Read group descriptor block and find inode table
	getblk(2, buf1);
	gp = (GD *)buf1;
	iblk = (u16)gp->bg_inode_table;

	getblk((u16)iblk, buf1);  // read first inode block
	ip = (INODE *)buf1 + 1;   // ip->root inode #2 ( / directory)

	// Read through directory entries looking for /boot
	i = findentry((u16)ip->i_block[0], "boot");

	// get inode
	ip = getinode(iblk, i);

	i = findentry((u16)ip->i_block[0], kstr);

	// Get inode
	ip = getinode(iblk, i);

	// get single indirect blocks
	getblk((u16)ip->i_block[12], buf2);
	
	// Copy file into memory
	prints("Loading: ");
	setes(0x1000);
	loc = 0;
	i = 0;

	// Get first 12 blocks
	while(i < 12 && ip->i_block[i] != 0)
	{
		getblk((u16)ip->i_block[i], loc);
		i++;
		loc += 1024;
		putc('.');
	}

	l = buf2;
	i = 0;
	while(i < 256 && l[i] > 0)
	{
		getblk((u16)(l[i]), loc);
		i++;
		loc += 1024;
		putc('+');
	}

	prints("\n\rReady?\n\r");
	getc();
	return 1;
}  
