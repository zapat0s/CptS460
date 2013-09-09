!================== bs.s file ===========================================
	BOOTSEG  =  0x9000		! Boot block is loaded again to here.
	OSSEG    =  0x1000
	SSP      =    8192
	
	.globl _main,_prints                             ! IMPORT symbols
	.globl _getc,_putc,_readfd,_setes,_inces,_error  ! EXPORT symbols
	                                                
	!-------------------------------------------------------
	! Only one SECTOR loaded at (0000,7C00). Get entire BLOCK in
	!-------------------------------------------------------
	mov  ax,#BOOTSEG
	mov  es,ax				! set ES to BOOTSEG=0x9000
	xor  bx,bx

	xor  dx,dx
	xor  cx,cx
	incb cl

	mov  ax,#0x0202
	int  0x13

	jmpi next,BOOTSEG
next:                    
	mov  ax,cs
	mov  ds,ax
	mov  ss,ax
	mov  sp,#SSP

	mov    ax,#0x0012		! set color mode
	int    0x10
	
	call _main				! call main() in C
     
	jmpi 0,OSSEG
 

!======================== I/O functions =================================
	!---------------------------------------
	! readfd(cyl, head, sector, buf)
	!        4     6     8      10
	!---------------------------------------
_readfd:                             
	push  bp
	mov   bp,sp				! bp = stack frame pointer

	movb  dl, #0x00			! drive 0=FD0
	movb  dh, 6[bp]			! head
	movb  cl, 8[bp]			! sector
	incb  cl
	movb  ch, 4[bp]			! cyl
	mov   bx, 10[bp]		! BX=buf ==> memory addr=(ES,BX)
	mov   ax, #0x0202		! READ 2 sectors to (EX, BX)

	int  0x13				! call BIOS to read the block 
	jb   _error				! to error if CarryBit is on [read failed]

	pop  bp                
	ret
		
	!---------------------------------------------
	!  char getc()   function: returns a char
	!---------------------------------------------
_getc:
	xorb   ah,ah			! clear ah
	int    0x16				! call BIOS to get a char in AX
	ret 

	!----------------------------------------------
	! void putc(char c)  function: print a char
	!----------------------------------------------
_putc:           
	push   bp
	mov    bp,sp
	
	movb   al,4[bp]			! get the char into aL
	movb   ah,#14			! aH = 14
	mov    bx,#0x000C		! bL = cyan color 
	int    0x10				! call BIOS to display the char

	pop    bp
	ret
        
_setes:  push  bp
	mov   bp,sp

	mov   ax,4[bp]        
	mov   es,ax

	pop   bp
	ret
	
_inces:						! inces() inc ES by 0x40
	mov   ax,es
	add   ax,#0x40
	mov   es,ax
	ret

	!------------------------------
	!       error & reboot
	!------------------------------
_error:
	push #msg
	call _prints

	int  0x19				! reboot

msg:    .asciz  "Error"
