/* ****************************************** */
/* HP-IL device emulation                     */
/* (used in Emu41/Emu71/ILper)                */
/*                                            */
/* ilser2.c   HP-IL serial interface module   */
/*                                            */
/* Version 2.5, J-F Garnier 2005-2007:        */
/* ****************************************** */

#include <stdio.h>
#include <conio.h>
#include <dos.h>



static char did[]="SERIAL2";  /* HP-IL Device ID */

extern char fserial;  /* flag serial port in use, for synchr with Emu41 auto-speed */


/* commun serial routines */
#include "ilserial.c"




/* ****************************************** */
/* init_ilserial2()                           */
/*                                            */
/* init the virtual HPIL serial 2 device      */
/* ****************************************** */
void init_ilserial2(int d, int config)
{
  init_ilserial(d,config);
}

/* ****************************************** */
/* ilserial2(frame)                           */
/*                                            */
/* manage a HPIL frame                        */
/* returns the returned frame                 */
/* ****************************************** */
int ilserial2(int frame)
{
  if ((frame&0x400)==0) frame=traite_doe(frame);
  else if ((frame&0x700)==0x400) frame=traite_cmd(frame);
  else if ((frame&0x700)==0x500) frame=traite_rdy(frame);

  return(frame);
}



