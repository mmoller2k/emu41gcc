/* ****************************************** */
/* HP-IL device emulation                     */
/* (used in Emu41/Emu71/ILper)                */
/*                                            */
/* ilext.c         external HP-IL module      */
/* (using the HP-IL board)                    */
/*                                            */
/* Version 2.5, J-F Garnier 1997-2007:        */
/* ****************************************** */

/* Conditional compilation:
  XIL: support of external HP-IL with HP-IL/PC board
  VERS_SLOW: for slow machines (186, 286)
  HPPLUS: specific for the HP Portable 110 plus
*/

#include <stdio.h>
#include <conio.h>
#include <dos.h>

#include "version.h"


#include "delai.h"


#ifdef XIL

static addr1LB3=0;
static int lastframe;
static int lbroken;
static char fready=0;


extern int hpil_transmit(int n, char f);  /* now in hpil.c */


/* ****************************************** */
/* wait100us()                                */
/*                                            */
/* 100us delay                                */
/* ****************************************** */
static void wait100us(void)
{
#ifndef VERS_SLOW
  int t, t0;

  t=readtimer();
  t0=multiplier/10;
  while ((readtimer()-t)<t0) ;
#endif
}

#ifdef HPPLUS
static void writereg(int addr, int r)
{
  outp(0x21+addr*2,r);
}

static int readreg(int addr)
{
  return(inp(0x21+addr*2));
}

#else

/* ****************************************** */
/* writereg(addr,r)                           */
/*                                            */
/* write to a 1LB3 chip register              */
/* ****************************************** */
static void writereg(int addr, int r)
{
  if (addr1LB3!=0) outp(addr1LB3+addr,r);
}

/* ****************************************** */
/* readreg(addr,r)                            */
/*                                            */
/* read from a 1LB3 chip register             */
/* ****************************************** */
static int readreg(int addr)
{
  if (addr1LB3!=0) return(inp(addr1LB3+addr)); else return 0;
}

#endif

/* ****************************************** */
/* init_ilext(addr,ctrl)                      */
/*                                            */
/* init the external HPIL interface board     */
/*   addr: address of the 1LB3 chip           */
/*   ctrl: flag controller mode               */
/* ****************************************** */
void init_ilext(int addr, int ctrl)
{
  if (addr!=0) addr1LB3=addr;

  if (addr1LB3==0) return;

  /* init 1LB3 IC */
  readreg(0);    /* wake-up 1LB3 */
  delay(1);
  writereg(0,1); /* master reset */
  delay(1);
  if (ctrl)
    writereg(0,128+64+16);  /* SC+CA+LA */
  else
    writereg(0,16);  /* LA */
  delay(1);
  writereg(3,0);   /* PPOLL=0 */
  delay(1);
  lastframe=-1;
  lbroken=0;
  if (addr) fready=1;

}

/* ****************************************** */
/* sendframe()                                */
/*                                            */
/* send a frame to the external HP-IL loop    */
/* returns the returned frame,                */
/*  or -1 if loop broken and IDY frame        */
/*  doesn't return                            */
/* (used in controller mode)                  */
/* ****************************************** */
static int sendframe(int frame)
{
  int i, imax;
  char c, c1;
  char s1;

  c=frame>>8;
  c1=lastframe>>8;

  /* if not IDY and loop broken, return immediatly */
  if ((c<6)&&lbroken) return(frame);

  if (c!=c1) {
    /* write reg1 */
    writereg(1,c<<5);
    wait100us();
  }

  /* write reg2 */
  writereg(2,frame);
  wait100us();

  /* wait for returned frame */
  i=0;
  if (c>=6) imax=100; /* timeout=10ms for IDY */
  else if (c==4) imax=10000;  /* CMD */
  else imax=32000;           /* RDY or DAT */
  lbroken=0;
  do {
    /* FRAV, FRNS or ORAV? */
    s1=readreg(1);
    wait100us();
    if (s1&6) {
      /* FRAV or FRNS : read frame */
      frame=s1&0xe0;
      frame=(frame<<3)+readreg(2);
      wait100us();
    }
    if (s1&7) break;
    i++;
    /* timeout */
    if (i>imax) {
      if (frame<0x600) frame=-1;  /* if not IDY, indicate error */
      lbroken=1;
      break;
    }
  } while (1);
  lastframe=frame;
  return(frame);
}



/* ****************************************** */
/* traite_extil()                             */
/*                                            */
/* manage the external HPIL loop when Emu41   */
/* is idle (device mode)                      */
/* ****************************************** */
void traite_extil(void)
{
  int s1, frame;

  if (!fready) return;

  /* wait for frame */
  /* FRAV, FRNS or IFCR ? */
    s1=readreg(1);
    wait100us();
    if (s1&0x16) {
      if (s1&0x10) {
        /* IFCR */
        frame=0x490;
      }
      else {
        /* FRAV or FRNS : read frame */
        frame=s1&0xe0;
        frame=(frame<<3)+readreg(2);
        wait100us();
      }

      frame=hpil_transmit(frame,0);

      if (frame==0x490) {
        /* IFC: reset IFCR */
        writereg(0,0x16);
        wait100us();
      }
      else if ((frame&0x700)==0x400) {
        /* CMD: set LRDY */
        writereg(0,0x14);
        wait100us();
      }
      else {
        writereg(1,(frame>>3)&0xe0);
        wait100us();
        writereg(2,frame);
        wait100us();
      }
    }
}


/* ****************************************** */
/* ilext(frame)                               */
/*                                            */
/* manage a HPIL frame on the external HP-IL  */
/* returns the returned frame                 */
/* (controller mode)                          */
/* ****************************************** */
int ilext(int frame)
{

  frame=sendframe(frame);

  return(frame);
}



#endif
