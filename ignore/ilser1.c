/* ****************************************** */
/* HP-IL device emulation                     */
/* (used in Emu41/Emu71/ILper)                */
/*                                            */
/* ilser1.c   HP-IL serial interface module   */
/*                                            */
/* Version 2.5, J-F Garnier 2005-2007:        */
/* ****************************************** */

#include <stdio.h>
#include <conio.h>
#include <dos.h>



static char did[]="SERIAL1";  /* HP-IL Device ID */

char fserial;         /* flag serial port in use, for synchr with Emu41 auto-speed */


/* commun serial routines */
#include "ilserial.c"



/* exported routines (used in Emu41p only for xmodem support) */

int send_car1(char car)
{
  return(send_car(car));
}

int read_car1(void)
{
  return(read_car());
}




/* ****************************************** */
/* init_ilserial1()                           */
/*                                            */
/* init the virtual HPIL serial 1 device      */
/* ****************************************** */
void init_ilserial1(int d, int config)
{
  init_ilserial(d,config);
}

/* ****************************************** */
/* ilserial1(frame)                           */
/*                                            */
/* manage a HPIL frame                        */
/* returns the returned frame                 */
/* ****************************************** */
int ilserial1(int frame)
{
  if ((frame&0x400)==0) frame=traite_doe(frame);
  else if ((frame&0x700)==0x400) frame=traite_cmd(frame);
  else if ((frame&0x700)==0x500) frame=traite_rdy(frame);

  return(frame);
}



