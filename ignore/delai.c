/* delai.c
   gestion du timer hardware pour mesure de temps pr�cise

   modif: JFG 24/03/97 remplace "pushf cli ... popf" par appels DPMI
		       pour compatibilite avec DPMI Windows
	  JFG 03/06/97 remplace appels a dummy par jmp
	  JFG 07/08/02 adapted for Emu41

  d'apr�s :

 *      C/C++ Run Time Library - Version 5.0
 *
 *      Copyright (c) 1987, 1992 by Borland International
 *      All Rights Reserved.
 *
 */



unsigned long multiplier = 1193*2L;


/*--------------------------------------------------------------------------*

Name            readtimer - read the complemented value of timer 0

Usage           unsigned readtimer (void);

Prototype in    local

Description     Obtain the complement of the value in timer 0.  The
		complement is used so that the timer will appear to
		count up rather than down.  The value returned will
		range from 0 to 0xffff.

Return value    The complement of the value in timer 0.

*---------------------------------------------------------------------------*/

unsigned readtimer (void)
{
  asm pushf                    /* Save interrupt flag                       */
  asm cli                      /* Disable interrupts                        */
  asm mov  al,0h               /* Latch timer 0                             */
  asm out  43h,al
  asm jmp  rt1
  asm nop
rt1:
  asm jmp  rt2
rt2:
  asm in   al,40h              /* Counter --> bx                            */
  asm mov  bl,al               /* LSB in BL                                 */
  asm jmp  rt3
  asm nop
rt3:
  asm jmp  rt4
rt4:
  asm in   al,40h
  asm mov  bh,al               /* MSB in BH                                 */
  asm not  bx                  /* Need ascending counter                    */
  asm popf                     /* Restore interrupt flag                    */
  return( _BX );
}


/*--------------------------------------------------------------------------*

Name            timer_init - initialize multiplier for delay function

Usage           void timer_init (void);

Prototype in    local

Description     Determine the multiplier required to convert milliseconds
		to an equivalent interval timer value.  Interval timer 0
		is normally programmed in mode 3 (square wave), where
		the timer is decremented by two every 840 nanoseconds;
		in this case the multiplier is 2386.  However, some
		programs and device drivers reprogram the timer in mode 2,
		where the timer is decremented by one every 840 ns; in this
		case the multiplier is halved, i.e. 1193.

		When the timer is in mode 3, it will never have an odd value.
		In mode 2, the timer can have both odd and even values.
		Therefore, if we read the timer 100 times and never
		see an odd value, it's a pretty safe assumption that
		it's in mode 3.  This is the method used in timer_init.

Return value    None

*---------------------------------------------------------------------------*/

void timer_init(void)
{
    int i;

    for (i = 0; i < 100; i++)
	if ((readtimer() & 1) == 0)     /* readtimer() returns complement */
	    {
	    multiplier = 1193L;
	    return;
	    }
}


/*--------------------------------------------------------------------------*

Name            delay - wait for specified period.

Usage           void delay(unsigned milliseconds);

Prototype in    dos.h

Description     The current thread of execution is suspended for the specified
		number of milliseconds.

Return value    None
*---------------------------------------------------------------------------*/

void delay( unsigned milliseconds )
{
    unsigned long stop;
    unsigned cur, prev;

    stop = (prev = readtimer()) + (milliseconds * multiplier);

    while ((cur = readtimer()) < stop)
	{
	if (cur < prev)     /* Check for timer wraparound */
	    {
	    if (stop < 0x10000L)
		break;
	    stop -= 0x10000L;
	    }
	prev = cur;
	}
}
