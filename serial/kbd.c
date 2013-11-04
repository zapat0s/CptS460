/************** A simple KBD driver *************************
To demo a simple KBD driver: handle lower case ASCII key press only
No shift, esc, etc.
Intended extensions: 
   handle shift keys. esc keys add function keys
*********************************************************/

#define KEYBD   0x60	/* I/O port for keyboard data */
#define PORT_B  0x61    /* port_B of 8255 */
#define KBIT	0x80	/* bit used to ack characters to keyboard */

#define KBSIZE    64    // size of input buffer in bytes
#define N_SCAN   105	/* Number of scan codes */

/* Scan codes to ASCII for unshifted keys; unused keys are left out */
char unshift[N_SCAN] = { 
 0,033,'1','2','3','4','5','6',        '7','8','9','0','-','=','\b','\t',
 'q','w','e','r','t','y','u','i',      'o','p','[',']', '\r', 0,'a','s',
 'd','f','g','h','j','k','l',';',       0, 0, 0, 0,'z','x','c','v',
 'b','n','m',',','.','/', 0,'*',        0, ' '       
};

/* Scan codes to ASCII for shifted keys; unused keys are left out */
char shift[N_SCAN] = {
 0,033,'!','@','#','$','%','^',        '&','*','(',')','_','+','\b','\t',
 'Q','W','E','R','T','Y','U','I',      'O','P','{','}', '\r', 0,'A','S',
 'D','F','G','H','J','K','L',':',       0,'~', 0,'|','Z','X','C','V',
 'B','N','M','<','>','?',0,'*',         0, ' '  
};

int kbdata;            // has KBD input flag
char kbc;              // data char

int kbinit()
{
  printf("kbinit() : ");
  kbdata = 0;          // flag = 0 initially
  enable_irq(1);
  out_byte(0x20, 0x20);
  printf("kbinit done\n\r");
}

/************************************************************************
 kbhandler() is the kbd interrupt handler. PC's kbd generates 2 interrupts
 for each key typed; one when the key is pressed and another one when the
 key is released. Each key generates a scan code. The scan code of a key
 release is 0x80 + the scan code of key pressed. When the kbd interrupts,
 the scan code is in the data port (0x60) of the KBD interface. First, 
 read the scan code from the data port. Then ack the key input by strobing 
 its PORT_B at 0x61.
 Some special keys generate ESC key sequences,e.g. arrow keys.

                   Then process the scan code:
1. Ignore key releases, thus no special keys, such as shift. 
   For normal keys: translate into LOWER CASE ASCII char.
2. Raise the kbdata flag, allowing a busy waiting process to get the key
************************* Lower half driver ******************************/
int kbhandler()
{
  int scode, value, c;

  /* Fetch the character from the keyboard hardware and acknowledge it. */
  scode = in_byte(KEYBD);/* get the scan code of the key struck */
  value = in_byte(PORT_B);/* strobe the keyboard to ack the char */
  out_byte(PORT_B, value | KBIT);/* first, strobe the bit high */
  out_byte(PORT_B, value); /* then strobe it low */

  printf("kb interrupt %x\n", scode);  // should see the scand code
  if (scode & 0x80)                    // ignore key release
    goto out;

  kbc = unshift[scode];                // translate scan code into ASCII char

  kbdata = 1;                          // raise kbdata flag

 out:
   out_byte(0x20, 0x20); 
}

/**************************** upper half driver ***********************/ 
// process call getc() to return a char from KBD, by busy waiting loop
int getc()
{
  char c;
 
  unlock();          // syscall to MTX kernel MAY have masked out interrupts

  while(kbdata==0);  // busy waiting loop for kbdata = 1

  c = kbc & 0x7F;// in general, get char from a COMMON data area, e.g. a kbbuf[]
  lock();
   kbdata = 0;       // this is a CR between process and interrupt
  unlock();
  return c;
}
