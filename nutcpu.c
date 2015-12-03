/* ****************************************** */
/* Emu41: HP-41C software emulator for PC     */
/*                                            */
/* nutcpu.c   NUT CPU simulator module        */
/*                                            */
/* Version 2.5, J-F Garnier 1997-2007         */
/* ****************************************** */

/* Conditional compilation:
   VERS_ASM: use nutcpu2.asm
   FPU: support of FPU (not implemented)
*/


#include <mem.h>
#include <stdio.h>
#include <dos.h>

/* global variables */
#define GLOBAL extern
#include "nutcpu.h"

#include "display.h"
#include "timer.h"
#include "hpil.h"

#define LOCAL static near

/* delay for LCD stable before detecting LCD change (unit=opcode counter) */
#define LCDDELAY 100  
/* delay for LCD off stable before detecting LCD off */ 
#define LCDOFFDELAY 500


/* field code table */
static int coded[16] = {3,4,5,10,8,6,11,15,2,9,7,13,1,12,0,14 };



/* ****************************************** */
/* int fetch1(unsigned int adr)               */
/*                                            */
/* fetch one opcode from address adr          */
/* ****************************************** */
int fetch1(unsigned int adr)
{
  int p0, n;
  static short *ptr=NULL;
  static unsigned int lastaddr=0xffff;

  /* lastaddr holds the last addr + 1 to avoid pointer calculations
     when fetching successive words */
  if (adr!=lastaddr) {
    lastaddr=adr;
    p0=adr>>12; /* page number*/
    adr &=0xfff;
    if (tabpage[p0]==0) {
      /* nonexistent page */
      lastaddr=0xffff;
      return(0);
    }
    /* setup pointer */
    //ptr= (int far *) MK_FP(tabpage[p0],adr<<1);
    ptr = tabpage[p0]+adr;
  }

  n=*ptr++;
  lastaddr++;
  return(n);
}



/* ****************************************** */
/* wmldl1(adr, mot)                           */
/*                                            */
/* write one word at address adr              */
/* in code space                              */
/* ****************************************** */
void wmldl1(unsigned int adr, int mot)
{
  int p0;
  unsigned short *ptr;

  p0=adr>>12; /* page number */
  adr &=0xfff;
  if (tabpage[p0]==0) return;
  //ptr= (unsigned int far *) MK_FP(tabpage[p0],adr<<1);
  ptr = tabpage[p0]+adr;
  *ptr = mot;
}





/* ****************************************** */
/* int execp(int mot)                         */
/*                                            */
/* execute the smart peripheral opcodes       */
/* (HPIL and HP82143)                         */
/* ****************************************** */
int execp(int mot)
{
  int n, ret;

  ret=0;
  Carry=0;

  if (selper<8) {
    /* HPIL */
    switch (mot&3) {
      case 1: /* CH= n */
        hpil_wr(selper,mot>>2);
        break;
      case 2: /* RDPTRN n */
        if (((mot>>2)&0x0e)==0x0e) {
          n=hpil_rd(selper);
          memset(regC,0,14);
          regC[0]=n&15;
          regC[1]=n>>4;
        }
        break;
      case 3: /* ?PFSET n */
        break;
    } /* end case */
  }

  else if (selper==9) {
    /* HP82143A printer */
    if ((mot&7)==7) {
      /* LOAD */
      print_char((regC[1]<<4)+regC[0]);
    }
    else if ((mot&0x3f)==0x3a) {
      /* SAVE */
      memset(regC,0,14);
      n=get_printer_status();
      if (n!=0) {
        regC[13]=(n>>12)&15;
        regC[12]=(n>>8)&15;
        regC[11]=(n>>4)&15;
        regC[10]=n&15;
      }
    }
    else if ((mot&0x3f)==0x03) {
      /* SET */
      Carry=test_printer_flag((mot>>6)&3);
    }
  }

  /* test if last smart opcodes */
  if (mot&1) {
    smartp=0;
  }
  return(ret);
}

#ifndef VERS_ASM

/* ****************************************** */
/* int popaddr()                              */
/*                                            */
/* pop an address from stack                  */
/* ****************************************** */
int LOCAL popaddr(void)
{
  int a;

  a=retstk[0];
  retstk[0]=retstk[1];
  retstk[1]=retstk[2];
  retstk[2]=retstk[3];
  retstk[3]=0;
  return(a);
}

/* ****************************************** */
/* pushaddr(int a)                            */
/*                                            */
/* push an address to stack                   */
/* ****************************************** */
void LOCAL pushaddr(int a)
{
  retstk[3]=retstk[2];
  retstk[2]=retstk[1];
  retstk[1]=retstk[0];
  retstk[0]=a;
}

#endif

/* ****************************************** */
/* int fieldaddr()                            */
/*                                            */
/* decodes the address field from a register  */
/* ****************************************** */
int LOCAL fieldaddr(unsigned char *reg)
{
  int n;

  n=reg[3];
  n += reg[4]<<4;
  n += reg[5]<<8;
  n += reg[6]<<12;
  return(n);
}


/* ****************************************** */
/* int storeData(int adr)                     */
/*                                            */
/* store the C register into a RAM register,  */
/* or write to the selected peripheral device */
/* ****************************************** */
int storeData(int adr)
{
  unsigned char *ptr;
  unsigned char *reg;
  int i, n, ret;

  ret=0;
  reg=regC;

  /* RAM register */
  n=regData&0x3ff;      /* bug fix 11/10/05, thanks to Simon S. */
  ptr=&espaceRAM[n*8];
  if ((*ptr++)==0) {
    for (i=0;i<7;i++) {
      *ptr =  (*reg++);
      *ptr += (*reg++) <<4;
      ptr++;
    }
  }

  if (regPer==0xfd) {
    /* display */
    if (adr==-1)
      display_wr();
    else
      display_wr_n(adr);
    if (dspon) facces_dsp=LCDDELAY;
  }
  
  if (regPer==0xfb) {
    /* clock */
    if (adr==-1)
      timer_wr_n(0);
    else
      timer_wr_n(adr);
  }

  return(ret);
}


/* ****************************************** */
/* int recallData(int adr)                    */
/*                                            */
/* read a RAM register into C,                */
/* or read the selected peripheral device     */
/* ****************************************** */
int recallData(int adr)
{
  unsigned char *ptr;
  unsigned char *reg;
  int i, n, ret;


  ret=0;
  switch (regPer) {
    case 0xfe:
      /* optical wand */
      break;
    case 0xfd:
      /* display */
      display_rd_n(adr);
      if (dspon&&(adr>2)&&(adr!=5)) facces_dsp=LCDDELAY;  /* added adr tests JFG 29/12/03 */
      break;
    case 0xfc:
      /* card reader */
      reg=regC;
      for (i=0;i<7;i++) {
        *reg++ = 0;
        *reg++ = 0;
      }
      break;
    case 0xfb:
      /* clock */
      timer_rd_n(adr);
      break;
    default:
      /* RAM register */
      reg=regC;
      n=regData&0x3ff;      /* bug fix 11/10/05, thanks to Simon S. */
      ptr=&espaceRAM[n*8];
      if ((*ptr++)==0) {
        for (i=0;i<7;i++) {
          *reg++ = (*ptr) & 15;
          *reg++ = (*ptr)>>4;
          ptr++;
        }
      }
      else {
        for (i=0;i<7;i++) {
          *reg++ = 0;
          *reg++ = 0;
        }
      }
  } /* end switch */

  return(ret);
}

/* ****************************************** */
/* enrom(int page, int n)                     */
/*                                            */
/* enable a block of a page                   */
/* ****************************************** */
void enrom(int page,int n)
{
  int i;

  page &=15;
  switch (typmod[page]) {
    case 2:
      /* CXTIME */
      if ((n==0)||(n==1)) tabpage[5]=tabbank[5][n];
      break;
    case 3:
      /* Advantage */
      if ((n==0)||(n==1)) tabpage[page|1]=tabbank[page|1][n];
      break;
    case 4:
     /* HEPAX */
     tabpage[page]=tabbank[page][n];
     break;
    case 6:
     /* IR PRINTER */
     if ((n==0)||(n==1)) tabpage[6]=tabbank[6][n];
     break;
    case 8:
    case 9:
     /* W&W RAMBOX II */
     if (n==0) {
       /* reset pages to bloc A */
       for (i=8;i<16;i++) {
         if ((typmod[i]==8)||(typmod[i]==9))
           tabpage[i]=tabbank[i][0];
       }
     }
     else if (n==1) {
       /* enable odd pages from block B */
       for (i=9;i<16;i+=2) {
         if ((typmod[i]==8)||(typmod[i]==9))
           tabpage[i]=tabbank[i][1];
       }
     }
     else if (n==2) {
       /* enable even pages from block B */
       for (i=8;i<16;i+=2) {
         if ((typmod[i]==8)||(typmod[i]==9))
           tabpage[i]=tabbank[i][1];
       }
     }
     break;
  }
  fetch1(0); /* just to reset addr calculations */
}




/* ****************************************** */
/* int wmldl()                                */
/*                                            */
/* write the C register into the code space   */
/* ****************************************** */
void wmldl(void)
{
  unsigned int adr;
  int i, n, typ;
  static int esrsu=0;

  adr=fieldaddr(regC);
  n=regC[0]+(regC[1]<<4)+(regC[2]<<8);
  n &= 0x3ff;
  if (adr==0) {
    /* Eramco ES RSU special case: writing to address 0 switches RSU pages */
    switch (n) {
      case 0x006: esrsu &= 2; break;
      case 0x007: esrsu &= 1; break;
      case 0x106: esrsu |= 1; break;
      case 0x107: esrsu |= 2; break;
    }
    for (i=8;i<=13;i++) {
      if ( (typmod[i]==10)||(typmod[i]==11) )
        tabpage[i]=tabbank[i][esrsu];
    }
  }
  else {
    typ=typmod[adr>>12];
    /* test if the page is writable */
    if ( ( (typ==5) && (fhram[adr>>12]==0) )
        || ( typ==7 )
        || ( typ==9 )
        || ( typ==11 ) ) {
      wmldl1(adr,n);
    }
  }
}



/* ****************************************** */
/* dokey()                                    */
/*                                            */
/* manage the state machine for virtual keys  */
/* ****************************************** */
void dokey(void)
{
  int i;

  switch (flagKey) {
    case 0: /* idle state, wait for a key*/
      if (lgkeybuf) {
        /* at least a key available, simulates a key pressed */
        regK=keybuffer[0];
        flagKB=1;         /* 03/01/04 */
        for (i=1;i<lgkeybuf;i++)
          keybuffer[i-1]=keybuffer[i];
        lgkeybuf--;
        flagKey=1; /* key pressed */
        cptKey=cptinstr;
      }
      break;
    case 1: /* pressed key state, wait for key released */
      if ((cptinstr-cptKey)>200) {
        flagKey=2;  /* key released */
        cptKey=cptinstr;
      }
      break;
    case 2: /* released, wait for next key to be pressed */
      if ((cptinstr-cptKey)>200) {
        flagKey=0;  /* idle */
        cptKey=cptinstr;
      }
      break;
  } /* end case */
}

/* ****************************************** */
/* int exec0(int mot)                         */
/*                                            */
/* type 0 opcode execute                      */
/* ****************************************** */
int LOCAL exec0(int mot)
{
  int par;
  int ret;
  int i, n;
  char buf[16];

  par=mot>>6;

  ret=0;

  switch (mot>>2) {

    case 0: /* NOP */
      Carry=0;
      break;
    case 1*16: /* WMLDL */
      Carry=0;
      wmldl();
      if (breakcode==2) ret=3;  /* 04/01 */
      fjmp=1;  /* 30/10/05 for ES-RSU */
      break;
    case 2*16:
      ret=3; /* breakpoint */
      break;
    case 3*16: /* FPU extension (Emu41) - not implemented */
#ifdef FPU
      n=fetch1(regPC);
      regPC++;
      ret=FPUext(n);
#endif
      Carry=0;
      break;
    case 4*16:  /* ENROM1 */
      enrom(regPC>>12,0);
      break;
    case 5*16:  /* ENROM3 (HEPAX) */
      enrom(regPC>>12,2);
      break;
    case 6*16:  /* ENROM2 */
      enrom(regPC>>12,1);
      break;
    case 7*16:  /* ENROM4 (HEPAX) */
      enrom(regPC>>12,3);
      break;
    case 8*16:
    case 9*16:
    case 10*16:
    case 11*16:
    case 12*16:
    case 13*16:
    case 14*16:
    case 15*16:
      /* HPIL=C n */
      par -=8;
      hpil_wr(par,(regC[1]<<4)+regC[0]);
      Carry=0;
      break;

#ifndef VERS_ASM      
      
    case 0*16+1:
    case 1*16+1:
    case 2*16+1:
    case 3*16+1:
    case 4*16+1:
    case 5*16+1:
    case 6*16+1:
    case 7*16+1:
    case 8*16+1:
    case 9*16+1:
    case 10*16+1:
    case 11*16+1:
    case 12*16+1:
    case 13*16+1:
    case 14*16+1:
      regST &= ~(1<<coded[par]);    /* ST=0 n */
      Carry=0;
      break;
    case 15*16+1:
      regST &=0xff00;               /* CLRST */
      Carry=0;
      break;


    case 0*16+2:
    case 1*16+2:
    case 2*16+2:
    case 3*16+2:
    case 4*16+2:
    case 5*16+2:
    case 6*16+2:
    case 7*16+2:
    case 8*16+2:
    case 9*16+2:
    case 10*16+2:
    case 11*16+2:
    case 12*16+2:
    case 13*16+2:
    case 14*16+2:
      regST |= (1<<coded[par]);     /* ST=1 n */
      Carry=0;
      break;
    case 15*16+2:                  /* RSTKB */
      dokey();  /* manage the key state macchine */
      if (flagKey!=1) { 
        flagKB=0;  regK=0;   /* no key down, reset internal registers */
      }
      Carry=0;
      break;

    case 0*16+3:
    case 1*16+3:
    case 2*16+3:
    case 3*16+3:
    case 4*16+3:
    case 5*16+3:
    case 6*16+3:
    case 7*16+3:
    case 8*16+3:
    case 9*16+3:
    case 10*16+3:
    case 11*16+3:
    case 12*16+3:
    case 13*16+3:
    case 14*16+3:
      Carry=((regST>>coded[par])&1); /* ?ST=1 n */
      break;
    case 15*16+3:   /* CHKKB */
      dokey();  /* manage the key state macchine */
      Carry=flagKB;
      break;

    case 0*16+4:
    case 1*16+4:
    case 2*16+4:
    case 3*16+4:
    case 4*16+4:
    case 5*16+4:
    case 6*16+4:
    case 7*16+4:
    case 8*16+4:
    case 9*16+4:
    case 10*16+4:
    case 11*16+4:
    case 12*16+4:
    case 13*16+4:
    case 14*16+4:
    case 15*16+4:
      /* LC  h */
      n=regPQ[regPT];
      regC[n]=par;
      n--;
      if (n<0) n=13;
      regPQ[regPT]=n;
      Carry=0;
      break;

    case 0*16+5:
    case 1*16+5:
    case 2*16+5:
    case 3*16+5:
    case 4*16+5:
    case 5*16+5:
    case 6*16+5:
    case 7*16+5:
    case 8*16+5:
    case 9*16+5:
    case 10*16+5:
    case 11*16+5:
    case 12*16+5:
    case 13*16+5:
    case 14*16+5:
      Carry=(regPQ[regPT]==coded[par]);  /* ?PT= n */
      break;
    case 15*16+5:
      n=regPQ[regPT];        /* DECPT */
      n--;
      if (n<0) n=13;
      regPQ[regPT]=n;
      Carry=0;
      break;

    case 1*16+6:     /* G=C */
      n=regPQ[regPT];
      regG=regC[n];
      n++; if (n>13) n=0;
      regG += regC[n]<<4;
      Carry=0;
      break;
    case 2*16+6:     /* C=G */
      n=regPQ[regPT];
      regC[n]=regG&15;
      n++; if (n>13) n=0;
      regC[n]=(regG>>4)&15;
      Carry=0;
      break;
    case 3*16+6:     /* CGEX */
      n=regPQ[regPT];
      i=regG;
      regG=regC[n];
      regC[n]=i&15;
      n++; if (n>13) n=0;
      regG += regC[n]<<4;
      regC[n]=(i>>4)&15;
      Carry=0;
      break;
    case 5*16+6:     /* M=C */
      memcpy(regM,regC,14);
      Carry=0;
      break;
    case 6*16+6:     /* C=M */
      memcpy(regC,regM,14);
      Carry=0;
      break;
    case 7*16+6:     /* CMEX */
      memcpy(buf,regC,14);
      memcpy(regC,regM,14);
      memcpy(regM,buf,14);
      Carry=0;
      break;
    case 9*16+6:     /* F=SB */
      regFO=regST&0xff;
      Carry=0;
      break;
    case 10*16+6:    /* SB=F */
      regST &= 0xff00;
      regST |= regFO;
      Carry=0;
      break;
    case 11*16+6:    /* FEXSB */
      n=regFO;
      regFO=regST&0xff;
      regST &= 0xff00;
      regST |= n;
      Carry=0;
      break;
    case 12*16+6:       /* ?? in SV Forth ... */
      Carry=0;
      break;
    case 13*16+6:    /* ST=C */
      regST &= 0xff00;
      regST |= regC[0]+(regC[1]<<4);
      Carry=0;
      break;
    case 14*16+6:    /* C=ST */
      regC[0]=regST&15;
      regC[1]=(regST>>4)&15;
      Carry=0;
      break;
    case 15*16+6:    /* CSTEX */
      n=regST;
      regST &= 0xff00;
      regST |= regC[0]+(regC[1]<<4);
      regC[0]=n&15;
      regC[1]=(n>>4)&15;
      Carry=0;
      break;
    case 0*16+6:
    case 4*16+6:
    case 8*16+6:
      ret=2;
      Carry=0;
      break;

    case 0*16+7:
    case 1*16+7:
    case 2*16+7:
    case 3*16+7:
    case 4*16+7:
    case 5*16+7:
    case 6*16+7:
    case 7*16+7:
    case 8*16+7:
    case 9*16+7:
    case 10*16+7:
    case 11*16+7:
    case 12*16+7:
    case 13*16+7:
    case 14*16+7:
      regPQ[regPT]=coded[par];  /* PT= n */
      Carry=0;
      break;
    case 15*16+7:
      n=regPQ[regPT];        /* INCPT */
      n++;
      if (n>13) n=0;
      regPQ[regPT]=n;
      Carry=0;
      break;

    case 0*16+8:   /* SPOPND */
      popaddr();
      Carry=0;
      break;
    case 1*16+8:   /* POWOFF */
      Carry=(dspon==0);
/*      flagKB=0;  03/01/04 */
      ret=1;
      break;
    case 2*16+8:   /* SELP */
      regPT=0;
      Carry=0;
      break;
    case 3*16+8:   /* SELQ */
      regPT=1;
      Carry=0;
      break;
    case 4*16+8:   /* ?P=Q */
      Carry= (regPQ[0]==regPQ[1]);
      break;
    case 5*16+8:   /* LLD */
      Carry=0;
      break;
    case 6*16+8:   /* CLRABC */
      memset(regA,0,14);
      memset(regB,0,14);
      memset(regC,0,14);
      Carry=0;
      break;
    case 7*16+8:   /* GOTOC */
      regPC=fieldaddr(regC);
      fjmp=1;
      Carry=0;
      break;
    case 8*16+8:   /* C=KEYS */
      dokey();  /* manage the key state macchine */
      regC[3]=regK&15;
      regC[4]=(regK>>4)&15;
      Carry=0;
      break;
    case 9*16+8:   /* SETHEX */
      flagdec=0;
      Carry=0;
      break;
    case 10*16+8:;  /* SETDEC */
      flagdec=1;
      Carry=0;
      break;
    case 11*16+8:;  /* DISOFF */
      if (dspon) facces_dsp=LCDOFFDELAY;  /* 24/12/03 */
      dspon=0;
      Carry=0;
      break;
    case 12*16+8:;  /* DISTOG */
      dspon=!dspon;
      facces_dsp=LCDDELAY;  /* 04/01/04  */
      Carry=0;
      break;
    case 13*16+8:;  /* RTNC */
      if (Carry) {
        regPC=popaddr();
        fjmp=1;
      }
      Carry=0;
      break;
    case 14*16+8:;  /* RTNNC */
      if (!Carry) {
        regPC=popaddr();
        fjmp=1;
      }
      Carry=0;
      break;
    case 15*16+8:;  /* RTN */
      regPC=popaddr();
      fjmp=1;
      Carry=0;
      break;

    case 0*16+9:      /* SELPF */
    case 1*16+9:
    case 2*16+9:
    case 3*16+9:
    case 4*16+9:
    case 5*16+9:
    case 6*16+9:
    case 7*16+9:
    case 8*16+9:
    case 9*16+9:
    case 10*16+9:
    case 11*16+9:
    case 12*16+9:
    case 13*16+9:
    case 14*16+9:
    case 15*16+9:
      selper=par;
      smartp=1;   /* enable smart periph. opcodes */
      Carry=0;
      if (breakcode&1)
        if (par==9)
          ret=3;
      break;

#endif
      
    case 0*16+10:	/* REGN=C n */
    case 1*16+10:
    case 2*16+10:
    case 3*16+10:
    case 4*16+10:
    case 5*16+10:
    case 6*16+10:
    case 7*16+10:
    case 8*16+10:
    case 9*16+10:
    case 10*16+10:
    case 11*16+10:
    case 12*16+10:
    case 13*16+10:
    case 14*16+10:
    case 15*16+10:
      regData &= 0xff0;
      regData |= par;
      ret=storeData(par);
      Carry=0;
      break;

    case 0*16+11:    /* ?Fx=1 */
    case 1*16+11:
    case 2*16+11:
    case 3*16+11:
    case 4*16+11:
    case 5*16+11:
    case 6*16+11:
    case 7*16+11:
    case 8*16+11:
    case 9*16+11:
    case 10*16+11:
    case 11*16+11:
    case 12*16+11:
    case 13*16+11:
    case 14*16+11:
    case 15*16+11:
      Carry=((regFI>>coded[par])&1);
      break;

    case 0*16+12:  /* ROMBLK (HEPAX) (or DISBLK in Voyager)*/
      for (i=3;i<16;i++) {
        if (typmod[i]==4) {
          n=regC[0];
          /* map bank HEPAX in page n */
          tabpage[n] = tabpage[i];
          tabbank[n][0]=tabbank[i][0];
          tabbank[n][1]=tabbank[i][1];
          tabbank[n][2]=tabbank[i][2];
          tabbank[n][3]=tabbank[i][3];
          typmod[n]=4;
          /* remap RAM HEPAX in the free page */
          tabpage[i&15]=tabpage[i|1]-512;
          tabbank[i][0]=0;
          tabbank[i][1]=0;
          tabbank[i][2]=0;
          tabbank[i][3]=0;
          typmod[i]=5;
          break;
        }
      }
      Carry=0;
      break;
#ifndef VERS_ASM
    case 1*16+12:  /* N=C */
      memcpy(regN,regC,14);
      Carry=0;
      break;
    case 2*16+12:  /* C=N */
      memcpy(regC,regN,14);
      Carry=0;
      break;
    case 3*16+12:  /* CNEX */
      memcpy(buf,regC,14);
      memcpy(regC,regN,14);
      memcpy(regN,buf,14);
      Carry=0;
      break;
    case 4*16+12:   /* LDI nnn */
      n=fetch1(regPC);
      regPC++;
      regC[0]=n&15;
      regC[1]=(n>>4)&15;
      regC[2]=(n>>8)&15;
      Carry=0;
      break;
    case 5*16+12:  /* STK=C */
      pushaddr(fieldaddr(regC));
      Carry=0;
      break;
    case 6*16+12:  /* C=STK */
      n=popaddr();
      regC[3]=n&15;
      regC[4]=(n>>4)&15;
      regC[5]=(n>>8)&15;
      regC[6]=(n>>12)&15;
      Carry=0;
      break;
#endif
    case 7*16+12:  /* WPTOG (HEPAX) */
      fhram[regC[0]] = !fhram[regC[0]];
      Carry=0;
      break;
#ifndef VERS_ASM
    case 8*16+12:  /* GOKEYS */
      regPC &= 0xff00;
      regPC |= regK;
      fjmp=1;
      Carry=0;
      break;
    case 9*16+12:  /* DADD=C */
      regData=regC[0]+(regC[1]<<4)+(regC[2]<<8);
      regPer=0;
      Carry=0;
      break;
    case 10*16+12: /* CLREGS on Voyager, but not implemented */
      Carry=0;
      break;
#endif
    case 11*16+12: /* DATA=C */
      ret=storeData(-1);
      Carry=0;
      break;
#ifndef VERS_ASM
    case 12*16+12: /* CXISA */
      n=fieldaddr(regC);
      n=fetch1(n);
      regC[0]=n&15;
      regC[1]=(n>>4)&15;
      regC[2]=(n>>8)&15;
      Carry=0;
      break;
    case 13*16+12: /* C=C!A */
      for (i=0;i<14;i++)
        regC[i] |= regA[i];
      Carry=0;
      break;
    case 14*16+12:; /* C=C&A */
      for (i=0;i<14;i++)
        regC[i] &= regA[i];
      Carry=0;
      break;
    case 15*16+12:; /* PFAD=C */
      regPer=regC[0]+(regC[1]<<4);
      Carry=0;
      break;
#endif

    case 1*16+14:       /*  C=REGN n */
    case 2*16+14:
    case 3*16+14:
    case 4*16+14:
    case 5*16+14:
    case 6*16+14:
    case 7*16+14:
    case 8*16+14:
    case 9*16+14:
    case 10*16+14:
    case 11*16+14:
    case 12*16+14:
    case 13*16+14:
    case 14*16+14:
    case 15*16+14:
      regData &= 0xff0;
      regData |= par;
    case 0*16+14:   /* C=DATA */
      ret=recallData(par);
      Carry=0;
      break;

#ifndef VERS_ASM
    case 0*16+15:    /* RCR n */
    case 1*16+15:
    case 2*16+15:
    case 3*16+15:
    case 4*16+15:
    case 5*16+15:
    case 6*16+15:
    case 7*16+15:
    case 8*16+15:
    case 9*16+15:
    case 10*16+15:
    case 11*16+15:
    case 12*16+15:
    case 13*16+15:
    case 14*16+15:
      memcpy(buf,regC,14);
      n=coded[par];;
      for (i=0;i<=13;i++) {
        if (n>13) n -=14;
        regC[i]=buf[n];
        n++;
      }
      Carry=0;
      break;
    case 15*16+15:  /* Display compensation (DISCMP) in Voyager */
      Carry=0;
      break;

#endif

    default:          /* ??? */
      ret=2;

  } /* end case */

  return(ret);

}


#ifndef VERS_ASM

/* ****************************************** */
/* int exec1(int mot)                         */
/*                                            */
/* type 1 opcode execute                      */
/* GOSUBNC, GOSUBC, GOLNC, GOLC               */
/* ****************************************** */
int LOCAL exec1(int mot)
{
  unsigned int a, b;

  b = fetch1(regPC);
  a = ((mot>>2)&0xff) + ((b>>2)<<8);
  regPC++;
  switch (b&3) {
    case 0:   /* GOSUBNC */
      if ( (!Carry) && (fetch1(a)!=0) ) {
        pushaddr(regPC);
        regPC=a;
        fjmp=1;
      }
      break;
    case 1:   /* GOSUBC */
      if ( Carry && (fetch1(a)!=0) ) {
        pushaddr(regPC);
        regPC=a;
        fjmp=1;
      }
      break;
    case 2:   /* GOLNC */
      if (!Carry) {
        regPC=a;
        fjmp=1;
      }
      break;
    case 3:   /* GOLC */
      if (Carry) {
        regPC=a;
        fjmp=1;
      }
      break;
  }
  Carry=0;
  return(0);
}


/* ****************************************** */
/* int exec2(int mot)                         */
/*                                            */
/* type 2 (arithmetic) opcode execute         */
/* ****************************************** */
int LOCAL exec2(int mot)
{
  int i, n, m;
  int p1, p2;
  unsigned char *regdest, *regsrc;
  unsigned char regtemp[14];

  /* decode field */
  switch ((mot>>2)&0x07) {
    case 0:     /* PT */
      p1=regPQ[regPT];
      p2=p1;
      break;
    case 1:     /* X */
      p1=0;
      p2=2;
      break;
    case 2:     /* WPT */
      p1=0;
      p2=regPQ[regPT];
      break;
    case 3:     /* W */
      p1=0;
      p2=13;
      break;
    case 4:     /* PQ */
      p1=regPQ[0];
      p2=regPQ[1];
      if (p2<p1) p2=13;
      break;
    case 5:     /* XS */
      p1=2;
      p2=2;
      break;
    case 6:     /* M */
      p1=3;
      p2=12;
      break;
    case 7:     /* S */
      p1=13;
      p2=13;
      break;
  }

  switch ((mot>>5)&0x1f) {
    case 0:     /* A=0 */
      regdest=regA;
      goto clrreg;
    case 1:     /* B=0 */
      regdest=regB;
      goto clrreg;
    case 2:     /* C=0 */
      regdest=regC;
clrreg:
      regdest += p1;
      for (i=p1;i<=p2;i++)
        *regdest++ = 0;
      Carry=0;
      break;
    case 3:     /* ABEX */
      regdest=regA;
      regsrc=regB;
      goto exgreg;
    case 4:     /* B=A */
      regdest=regB;
      regsrc=regA;
      goto movreg;
    case 5:     /* ACEX */
      regdest=regA;
      regsrc=regC;
      goto exgreg;
    case 6:     /* C=B */
      regdest=regC;
      regsrc=regB;
      goto movreg;
    case 7:     /* BCEX */
      regdest=regB;
      regsrc=regC;
exgreg:
      regdest += p1;
      regsrc += p1;
      for (i=p1;i<=p2;i++) {
        n = *regdest;
        *regdest++ = *regsrc;
        *regsrc++ = n;
      }
      Carry=0;
      break;
    case 8:     /* A=C */
      regdest=regA;
      regsrc=regC;
movreg:
      regdest += p1;
      regsrc += p1;
      for (i=p1;i<=p2;i++)
        *regdest++ = *regsrc++;
      Carry=0;
      break;
    case 9:     /* A=A+B */
      regdest=regA;
      regsrc=regB;
      goto addreg;
    case 10:    /* A=A+C */
      regdest=regA;
      regsrc=regC;
      goto addreg;
    case 11:    /* A=A+1 */
      regdest=regA;
      goto increg;
    case 12:    /* A=A-B */
      regdest=regA;
      regsrc=regB;
      goto subreg;
    case 13:    /* A=A-1 */
      regdest=regA;
      goto decreg;
    case 14:    /* A=A-C */
      regdest=regA;
      regsrc=regC;
subreg:
      Carry=0;
      if (flagdec) m=10; else m=16;
      regdest += p1;
      regsrc += p1;
      for (i=p1;i<=p2;i++) {
        n = *regdest - *regsrc++ - Carry;
        if (n<0) {
          Carry=1;
          n +=m;
          n &= 15;
        }
        else
          Carry=0;
       *regdest++ = n;
      }
      break;
    case 15:    /* C=C+C */
      regdest=regC;
      regsrc=regC;
      goto addreg;
    case 16:    /* C=C+A */
      regdest=regC;
      regsrc=regA;
addreg:
      Carry=0;
      if (flagdec) m=10; else m=16;
      regdest += p1;
      regsrc += p1;
      for (i=p1;i<=p2;i++) {
        n = *regdest + *regsrc++ + Carry;
        if (n>=m) {
          Carry=1;
          n -=m;
          n &= 15;
        }
        else
          Carry=0;
        *regdest++ = n;
      }
      break;
    case 17:    /* C=C+1 */
      regdest=regC;
increg:
      Carry=1;
      if (flagdec) m=10; else m=16;
      regdest += p1;
      for (i=p1;i<=p2;i++) {
        n = *regdest + Carry;
        if (n>=m) {
          Carry=1;
          n -=m;
        }
        else
          Carry=0;
        *regdest++ = n;
      }
      break;
    case 18:    /* C=A-C */
      Carry=0;
      if (flagdec) m=10; else m=16;
      regdest = &regC[p1];
      regsrc  = &regA[p1];
      for (i=p1;i<=p2;i++) {
        n = *regsrc++ - *regdest - Carry;
        if (n<0) {
          Carry=1;
          n +=m;
          n &= 15;
        }
        else
          Carry=0;
        *regdest++ = n;
      }
      break;
    case 19:    /* C=C-1 */
      regdest=regC;
decreg:
      Carry=1;
      if (flagdec) m=10; else m=16;
      regdest += p1;
      for (i=p1;i<=p2;i++) {
        n = *regdest - Carry;
        if (n<0) {
          Carry=1;
          n +=m;
        }
        else
          Carry=0;
        *regdest++ = n;
      }
      break;
    case 20:    /* C=-C */
      Carry=0;
      goto negreg;
    case 21:    /* C=-C-1 */
      Carry=1;
negreg:
      if (flagdec) m=10; else m=16;
      regdest = &regC[p1];
      for (i=p1;i<=p2;i++) {
        n = -(*regdest) - Carry;
        if (n<0) {
          Carry=1;
          n +=m;
          n &= 15;
        }
        else
          Carry=0;
        *regdest++ = n;
      }
      break;
    case 22:    /* ?B#0 */
      regdest=regB;
      goto tstreg;
    case 23:    /* ?C#0 */
      regdest=regC;
      goto tstreg;
    case 24:    /* ?A<C */
      regdest=regA;
      regsrc=regC;
      goto cmpreg;
    case 25:    /* ?A<B */
      regdest=regA;
      regsrc=regB;
cmpreg:
      memcpy(regtemp,regdest,14);
      regdest=regtemp;
      goto subreg;
    case 26:    /* ?A#0 */
      regdest=regA;
tstreg:
      Carry=0;
      regdest += p1;
      for (i=p1;i<=p2;i++) {
        if ((*regdest++)!=0) {
          Carry=1;
          break;
        }
      }
      break;
    case 27:    /* ?A#C */
      Carry=0;
      regdest = &regC[p1];
      regsrc  = &regA[p1];
      for (i=p1;i<=p2;i++) {
        if (*regsrc++ != *regdest++) {
          Carry=1;
          break;
        }
      }
      break;
    case 28:    /* ASR */
      regdest=regA;
      goto shrreg;
    case 29:    /* BSR */
      regdest=regB;
      goto shrreg;
    case 30:    /* CSR */
      regdest=regC;
shrreg:
      regdest += p1;
      for (i=p1;i<p2;i++) {
        *regdest = *(regdest+1);
        regdest++;
      }
      *regdest=0;
      Carry=0;
      break;
    case 31:    /* ASL */
      regdest = &regA[p2];
      for (i=p2;i>p1;i--) {
        *regdest = *(regdest-1);
        regdest--;
      }
      *regdest=0;
      Carry=0;
      break;
  }


  return(0);
}



/* ****************************************** */
/* int exec3(int mot)                         */
/*                                            */
/* type 3 opcode execute                      */
/* GOC, GONC                                  */
/* ****************************************** */
int LOCAL exec3(int mot)
{
  int a;

  a=mot>>3;
  if (a>63) a-=128;
  a--;
  if (mot&4) {        /* GOC */
    if (Carry)
      regPC += a;
  }
  else {
    if (!Carry)     /* GONC */
      regPC += a;
  }
  Carry=0;
  return(0);
}




/* ****************************************** */
/* int executeNUT(int n)                      */
/*                                            */
/* executes n CocoNUT opcodes                 */
/* returns 0 if OK, 1 if poweroff,            */
/*   2 if invalid, 3 if break                 */
/* ****************************************** */
int executeNUT(int n)
{
  int i;
  int mot;
  int ret;

  for (i=0;i<n;i++) {
    /* read 1 word */
    mot=fetch1(regPC);
    regPC++;
    /* execute the opcode */
    if (smartp) {
      ret=execp(mot);
    }
    else {
      switch (mot&3) {
        case 0: ret=exec0(mot); break;
        case 1: ret=exec1(mot); break;
        case 2: ret=exec2(mot); break;
        case 3: ret=exec3(mot); break;
      }
    }
    cptinstr++;
    if (ret) break;
    if (facces_dsp) {
      /* if access to LCD ... */
      facces_dsp--;
      if (facces_dsp==0) {
        /* LCD now needs an update */
        fdsp=1;
      }
    }
    if (fdsp) break;
  } /* next i */

  return(ret);
}


#else


int executeNUT(int n)
{
  int mot, ret, lst;

  lst=cptinstr+n;
  do {
    ret=executeNUT2(n);
    if (ret==4) {
      mot=fetch1(regPC);
      regPC++;
      if (smartp) {
        ret=execp(mot);
      }
      else {
        ret=exec0(mot);
      }
      cptinstr++;
    }
    if (ret) break;
    if (fdsp) break;
    n=lst-cptinstr;
  } while (n>0);
  return(ret);
}

#endif

