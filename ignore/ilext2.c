/* ****************************************** */
/* HP-IL device emulation                     */
/* (used in Emu41/Emu71/ILper)                */
/*                                            */
/* ilext2.c        external HP-IL module      */
/* manage a PIL-Box using a serial COM port   */
/*  ( PC version)                             */
/*                                            */
/* Version 2.5, J-F Garnier 2006-2009         */
/* ****************************************** */

/* Conditional compilation:
  VERS_SLOW: for slow machines (186, 286)
  VERS_LAP: disable HDRIVE2, FDRIVE1, LPRTER1, SERIAL2 (for Portable Plus and LX)
*/


#include <stdio.h>
#include <conio.h>
#include <dos.h>

#include "version.h"


#include "delai.h"



static int lasth=0;
static char lbroken=0;
static char fready=0;

static int comport=0;


extern int hpil_transmit(int n, char f);  /* now in hpil.c */

extern char fserial;         /* flag serial port in use, for synchr with Emu41 auto-speed */


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


/* ****************************************** */
/* send_car(car)                              */
/*                                            */
/* send a character to serial port            */
/*  use BIOS calls (INT 14)                   */
/* ****************************************** */
static int send_car(char car)
{
  _DX = comport;
  _AH = 1;
  _AL = car;
  geninterrupt(0x14);

    fserial=1;
  return(_AH);
}

/* ****************************************** */
/* read_car()                                 */
/*                                            */
/* read a character from serial port          */
/*  use BIOS calls (INT 14)                   */
/* ****************************************** */
static int read_car(void)
{
  unsigned int n;

  _DX = comport;
  _AH = 2;
  geninterrupt(0x14);
  n=_AX;
  if ((n&0x8000)==0) {
/*  if (n&0x100) { */
    return(n&255);
  }
  else {
    fserial=1;
    return(-1);
  }
}

/* ****************************************** */
/* status_serial()                            */
/*                                            */
/* get the serial port status                 */
/*  use BIOS calls (INT 14)                   */
/* ****************************************** */
static int status_serial(void)
{
  unsigned int n;

  _DX = comport;
  _AH = 3;                     /* status */
  geninterrupt(0x14);
  n=_AX;
  if (n&0x100)
    return(1);
  else
    return(0);
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
  char c, hbyt, lbyt;
  int s1;

  c=frame>>8;

  if ((frame&0x794)==0x494)
    /* special PIL-Box frames (JFG, 17June09) */
    lasth=0;
  else
    /* if not IDY and loop broken, return immediatly */
    if ((c<6)&&lbroken) return(frame);

  /* build low and high parts (2nd format) */
  hbyt=((frame>>6)&0x1e)|0x20;
  lbyt=frame|0x80;

//  sprintf(buffer,"send: %x %x\n",hbyt, lbyt);
//  disp_string(buffer);

  if (hbyt!=lasth) {
    send_car(hbyt);
    lasth=hbyt;
  }
  send_car(lbyt);

  /* wait for returned frame */
  i=0;
  if (c>=6) imax=3000; /* timeout=10ms for IDY */
  else if ((frame==0x494)||(frame==0x496)||(frame==0x497)) imax=3000;  /* CON/OFF */
  else if (c==4) imax=10000;  /* CMD */
  else imax=30000;           /* RDY or DAT */
  lbroken=0;
  do {
    s1=-1;
    if (status_serial())
      s1=read_car();
    else
      wait100us();
//    delay(1);
    if (s1>=0) {
      if ((s1&0xc0)==0) {
        /* high part */
        if (s1&0x20) {
          lasth=s1;
          send_car(13);   /* ack */
          i=0; /* restart timeout */
        }
      }
      else {
        /* low byte, build frame, according to format */
        if (s1&0x80)
          frame=((lasth&0x1e)<<6) + (s1&0x7f);
        else
          frame=((lasth&0x1f)<<6) + (s1&0x3f);
        break;
      }
    }
    i++;
    /* timeout */
    if (i>imax) {
      if (frame<0x600) frame=-1;  /* if not IDY, indicate error */
      lbroken=1;
      break;
    }
  } while (1);
  return(frame);
}



/* ****************************************** */
/* init_ilext2(d,ctrl)                        */
/*                                            */
/* init the serial port                       */
/*   d: COM nimber (1-4+)                     */
/*   ctrl: flag controller mode               */
/* ****************************************** */
void init_ilext2(int d, int ctrl)
{
  int frame, fr2;

  if (d!=0) {
    /* init COM port */
    comport=d-1;
    _DX = comport;
    _AL = 0xe3;  /* 9600bps, 8 bits, no parity, 2 stops */
    _AH = 0;
    geninterrupt(0x14);
    fready=1;
  }

  if (fready) {
    lasth=0;
    lbroken=0;
    if (status_serial()) read_car();
    if (ctrl==1)
      frame=0x496; /* CON */
    else if (ctrl==0)
      frame=0x497; /* COFF */
    else
      frame=0x494; /* TDIS , added JFG 21/06/09 for PILBox 1.2 */
    fr2=sendframe(frame);
    if (fr2!=frame) {
      if (status_serial()) read_car();
//      fr2=sendframe(frame);
      delay(10);
      if (status_serial()) read_car();
    }

  }

}




/* ****************************************** */
/* traite_extil2()                            */
/*                                            */
/* manage the external HPIL loop when Emu41   */
/* is idle (device mode)                      */
/* ****************************************** */

int traite_extil2(void)
{
  int n, s1, frame;
  char hbyt, lbyt;

  if (!fready) return(0);

  /* wait for frame */
  n=status_serial();
  if (n) {
    s1=read_car();
    if (s1>=0) {
      if ((s1&0xc0)==0) {
        /* high part */
        if (s1&0x20) {
          lasth=s1;
          send_car(13);   /* ack */
        }
      }
      else {
        /* low byte, build frame, according to format */
        if (s1&0x80)
          frame=((lasth&0x1e)<<6) + (s1&0x7f);
        else
          frame=((lasth&0x1f)<<6) + (s1&0x3f);

        frame=hpil_transmit(frame,0);

        /* send returned frame */

        /* build low and high parts (2nd format)*/
        hbyt=((frame>>6)&0x1e)|0x20;
        lbyt=frame|0x80;

        if (hbyt!=lasth) {
          send_car(hbyt);
          wait100us();  //?
          lasth=hbyt;
        }
        send_car(lbyt);

      }

    }
  }
  return(n);
}


/* ****************************************** */
/* ilext2(frame)                              */
/*                                            */
/* manage a HPIL frame on the external HP-IL  */
/* returns the returned frame                 */
/* (controller mode)                          */
/* ****************************************** */

int ilext2(int frame)
{

  frame=sendframe(frame);

  return(frame);
}




