/* timer parameters. */
#define LATCH_COUNT     0x00	/* cc00xxxx, c = channel, x = any */
#define SQUARE_WAVE     0x36	/* ccaammmb, a = access, m = mode, b = BCD */

/************************ NOTICE THE DIVISOR VALUE ***********************/
#define TIMER_FREQ   1193182L	/* timer frequency for timer in PC and AT */
#define TIMER_COUNT ((unsigned) (TIMER_FREQ/60)) /* initial value for counter*/

#define TIMER0       0x40
#define TIMER_MODE   0x43
#define TIMER_IRQ       0

#define TIMER_EVENT     42

int hour, min, sec, fdon, fdcount;
hour = 0;
min = 0;
sec = 0;

fdon = 0;
fdcount = 0;

int enable_irq(irq_nr) unsigned irq_nr;
{
  lock();
    out_byte(0x21, in_byte(0x21) & ~(1 << irq_nr));

}

/*===========================================================================*
 *				timer_init				     *
 *===========================================================================*/

ushort tick;

int timer_init()
{
  /* Initialize channel 0 of the 8253A timer to e.g. 60 Hz. */

  printf("timer init\n");
  tick = 0;

  out_byte(TIMER_MODE, SQUARE_WAVE);	/* set timer to run continuously */
  out_byte(TIMER0, TIMER_COUNT);	/* load timer low byte */
  out_byte(TIMER0, TIMER_COUNT >> 8);	/* load timer high byte */
  enable_irq(TIMER_IRQ); 
}

/*===========================================================================*
 *				timer_handler				     *
 *===========================================================================*/

int thandler()
{
	tick++; 
	tick %= 60;

	if (tick % 60 == 0)
	{
		// update time
		sec++;
		if(sec >= 60)
		{
			sec = 0;
			min++;
	
			if(min >= 60)
			{
				min = 0;
				hour++;

				if(hour >= 24)
				{
					hour = 0;
				}
			}
		}
		printTime();
		
		// switch processes
		if(inkmode == 1)
		{
			running->time--;
			if(running->time <= 0)
			{
				out_byte(0x20, 0x20);	
				tswitch();
			}
		}
		
		// Wakeup sleeping procs
		timerWakeup();

		fdcount++;
		if(fdcount % 5 == 0)
		{
			if(fdon)
				out_byte(0x0C, 0x3F2);
			else
				out_byte(0x1C, 0x3F2);
			fdcount = 0;
		}

	}
	out_byte(0x20, 0x20);  
}

int printTime()
{
	int tmprow, tmpcol; 
	tmprow = row;
	tmpcol = column;

	row = 23;
	column = 66;
	printf("        ");

	row = 24;
	column = 66;
	printf("%d:%d:%d", hour, min, sec);

	row = tmprow;
	column = tmpcol;

	return 1;
}

int timedSleep(n) int n;
{
	printf("%d", n);
	running->time = n;
	sleep(running);
}

int timerWakeup()
{
	PROC **cur, *first, *tar;
	cur = &sleepList;
	first = (*cur);

	while(*cur)
	{
		if((*cur)->status == SLEEP && (*cur)->event == (*cur))
		{
			(*cur)->time--;
			printf("P%dwakes in %dseconds\n", (*cur)->pid, (*cur)->time);
			if((*cur)->time <= 0)
			{
				tar = (*cur);
				wakeup(tar);
			}
		}
		else
			cur = &(*cur)->next;
		if((*cur) == first)
			break;
	}
	return 1;
}

