/* ****************************************** */
/* Emu41: HP-41C software emulator for PC     */
/*                                            */
/* hpil.c   HP-IL emulation module            */
/*                                            */
/* Version 2.5, J-F Garnier 1997-2007         */
/* ****************************************** */

/* Conditional compilation:
   VERS_LAP: disable HDRIVE2, FDRIVE1, LPRTER1, SERIAL2 (for Portable Plus and LX)
   XIL: support of external HP-IL with HP-IL/PC board
*/

#include "version.h"

#define GLOBAL extern
#include "nutcpu.h"
#undef GLOBAL

#define GLOBAL
#include "hpil.h"
#undef GLOBAL




/* ****************************************** */
/* update_flags()                             */
/*                                            */
/* update HP41 FI register with HPIL flags    */
/* ****************************************** */
static void update_flags(void)
{
  regFI &= ~( 0x1f << 6);  /* mask HPIL flags */
  if (flgenb) {
    /* if HPIL flag enabled, update FI */
    /* flags register 1 :   4:IFCR, 3:SRQR, 2:FRAV, 1:FRNS, 0:ORAV */
    if (hpil_reg[1]&1)  regFI |= 1<<10;
    if (hpil_reg[1]&2)  regFI |= 1<<9;
    if (hpil_reg[1]&4)  regFI |= 1<<8;
    if (hpil_reg[1]&8)  regFI |= 1<<7;
    if (hpil_reg[1]&16) regFI |= 1<<6;
  }
}


/* ****************************************** */
/* init_hpil()                                */
/*                                            */
/* init the HPIL registers and flags          */
/* ****************************************** */
void init_hpil(void)
{
  int i;

  for (i=0;i<9;i++) hpil_reg[i]=0;
  hpil_reg[0]=0x81;  /* SC=1 MCL=1 */
  hpil_reg[1]=1;     /* ORAV */
  flgenb=0;
  update_flags();
}


/* ****************************************** */
/* hpil_transmit(n,f)                         */
/*                                            */
/* transmit a frame to the internal virtual   */
/* devices, flag f allows XIL/XIL2            */
/* returns the returned frame,                */
/*   or -1 if frame doesn't return            */
/* ****************************************** */
int hpil_transmit(int n, char f)
{
  int i;

  for (i=0;i<nbdev;i++) {
    if (n<0) break;   /* if loop broken, breaks */
    switch (tabdev[i]) {
      case 1: /* VIDEO */
        n=ildisplay(n);
        break;
      case 2: /* HDISC */
        n=ilhdisc(n);
        break;
      case 3: /* FDISC */
#ifndef VERS_LAP
        n=ilfdisc(n);
#endif
        break;
      case 4: /* DOS */
        n=ildos(n);
        break;
      case 5: /* XIL */
#ifdef XIL
        if (f) n=ilext(n);
#endif
        break;
      case 6: /* PRINTER */
#ifndef VERS_LAP
        n=ilcentro(n);
#endif
        break;
      case 7: /* HDISC2 */
#ifndef VERS_LAP
        n=ilhdisc2(n);
#endif
        break;
      case 8: /* SERIAL1 */
        n=ilserial1(n);
        break;
      case 9: /* SERIAL2 */
#ifndef VERS_LAP
        n=ilserial2(n);
#endif
        break;
      case 10: /* XIL2 */
#ifndef HPPLUS
        if (f) n=ilext2(n);
#endif
        break;
    }
  }
  return(n);
}


/* ****************************************** */
/* hpil_wr(reg, n)                            */
/*                                            */
/* write to a HPIL chip register              */
/*   (1LB3 simulation)                        */
/* ****************************************** */
void hpil_wr(int reg, int n)
{
  int n1;

  n &= 255;

  switch (reg) {

    case 0: /* status reg. */
      if (n&1) {
        /* MCL */
        hpil_reg[0] |= 0x80;  /* SC=1 */
        hpil_reg[1] &= 0xe1;  /* IFCR=SRQR=FRNS=FRAV=0 */
        hpil_reg[1] |= 1;     /* ORAV=1 */
        flgenb=0;
        update_flags();
      }
      if (n&2) {
        /* CLIFCR */
        hpil_reg[1] &= 0xef; /* IFCR=0 */
        update_flags();
      }
      hpil_reg[0]=n&0xfd;  /* CLIFCR is self clearing */
      break;

    case 1: /* control reg. */
      flgenb=n&1;
      hpil_reg[8] = n;       /* write to R1W */
      update_flags();
      break;

    case 2: /* data reg. */
      /* this is the core of the 1LB3 simulation */
      hpil_reg[2] = n;
      /* build frame */
      n |= (hpil_reg[8]&0xe0)<<3;      /* get bits 8-10 from R1W */

xfer:
      n1=hpil_transmit(n,1);        /* transmit frame */

      hpil_reg[1] &= 0xf7;      /* FRAV=FRNS=ORAV=0 */
      update_flags();
      if (n1<0) break;  /* in case frame didn't return */

      hpil_reg[2]=n1;   /* returned frame */
      hpil_reg[1] &= 0x1f;
      hpil_reg[1] |= (n1 & 0x700)>>3;  /* save received bits 8-10 to R1R */

      /* adjust flags according to received frame and state: */
      if ((n1&0x400)==0) {       /* DOE : 0xx */
        if (hpil_reg[0]&0x10) {  /* LA */
          hpil_reg[1] |= 5;      /* FRAV=ORAV=1 */
        }
        else if (hpil_reg[0]&0x20) {  /* TA */
          if ((n&0xff)!=(n1&0xff))
            hpil_reg[1] |= 3;    /* FRNS=ORAV=1 */
          else
            hpil_reg[1] |= 1;    /* ORAV=1 */
        }
        else {
          n=n1;             /* case LA=TA=0, 10/02/03 */
          goto xfer;        /* retransmit DOE frames ! */
        }
        if ((n1&0x100)!=0)
          hpil_reg[1] |= 8;      /* SRQR=1 */
        else
          hpil_reg[1] &= ~8;     /* SRQR=0 */
      }

      else if ((n1&0x200)!=0) {  /* IDY : 11x */
        if ((hpil_reg[0]&0x30)==0x30)  /* TA=LA=1 (scope mode) */
          hpil_reg[1] |= 5;      /* FRAV=ORAV=1 */
        else
          hpil_reg[1] |= 1;      /* ORAV=1 */
        if ((n1&0x100)!=0)
          hpil_reg[1] |= 8;      /* SRQR=1 */
        else
          hpil_reg[1] &= ~8;     /* SRQR=0 */
      }

      else if ((n1&0x100)==0) {  /* CMD : 100 */
        if ((hpil_reg[0]&0x30)==0x30)  /* TA=LA=1 (scope mode) */
          hpil_reg[1] |= 5;      /* FRAV=ORAV=1 */
        else if (n==n1)
          hpil_reg[1] |= 1;      /* ORAV=1 */
        else
          hpil_reg[1] |= 3;      /* FRNS=ORAV=1 */
        if (n1==0x490)           /* IFC */
          hpil_reg[1] |= 16;     /* IFCR=1 */
      }

      else {                     /* RDY : 101 */
        if ((hpil_reg[0]&0x30)==0x30)  /* TA=LA=1 (scope mode) */
          hpil_reg[1] |= 5;      /* FRAV=ORAV=1 */
        else if ((n1&0xc0)==0x40)   /* ARG */
          hpil_reg[1] |= 5;      /* FRAV=ORAV=1 */
        else {
          if (n==n1)
            hpil_reg[1] |= 1;    /* ORAV=1 */
          else
            hpil_reg[1] |= 3;    /* FRNS=ORAV=1 */
        }
      }

      update_flags();
      break;

    default:
      hpil_reg[reg] = n;
  }

}


/* ****************************************** */
/* hpil_rd(reg)                               */
/*                                            */
/* read from a HPIL chip register             */
/*   (1LB3 simulation)                        */
/* ****************************************** */
int hpil_rd(int reg)
{
  int n;

  switch (reg) {
    case 2:
      n=hpil_reg[1] & 6;    /* FRAV & FRNS */
      hpil_reg[1] &= 0xf9;  /* FRAV=FRNS=0 */
      if (n) hpil_reg[8]=hpil_reg[1];  /* copy R1R to R1W */
      update_flags();
    break;
  }

  return(hpil_reg[reg]);

}

