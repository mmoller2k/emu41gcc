/* ****************************************** */
/* Emu41: HP-41C software emulator for PC     */
/*                                            */
/* emu41.c   main module - PC version         */
/*                                            */
/* Version 2.5, J-F Garnier 1997-2009         */
/* ****************************************** */


/* Copyright (c) 1997-2009  J-F Garnier
'
' This program is free software; you can redistribute it and/or
' modify it under the terms of the GNU General Public License
' as published by the Free Software Foundation; either version 2
' of the License, or (at your option) any later version.
'
' This program is distributed in the hope that it will be useful,
' but WITHOUT ANY WARRANTY; without even the implied warranty of
' MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
' GNU General Public License for more details.
'
' You should have received a copy of the GNU General Public License
' along with this program; if not, write to the Free Software
' Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
'
' ------------------------------------------------------------------------- */


/* Conditional compilation:
  XIL: support of external HP-IL with HP-IL/PC board
*/

#include <stdio.h>
//#include <dos.h>
//#include <conio.h>
#include <mem.h>
#include <alloc.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "version.h"


extern int commande(void);  /* monit.c */


/* global variables */
/* welcome strings */
/*                1234567890123456789012345678901234567890 */
char *Welcome1 = "  Emu41: HP-41C & HP-IL system emulator              J-F GARNIER 1997, 2009    ";
char *Welcome2 = USER1;
char *Welcome3 = USER2;
char *Welcome4 = USER3;
int fmonit;     /* flag monitor mode */

/* commun variables to all modules */
#define GLOBAL
#include "nutcpu.h"

/* define the duration of slow speed before automatic switch to fast speed */
#define VALSLOW 400       /* 0.8 s */

/* local variables */
static int speed;        /* emulation speed */
static int fslowhost;    /* flag slow host (8086, 80286) */
static int fslow;        /* flag slow speed */
static int frun;         /* flag CPU running */
static int ffpu;         /* flag FPU used (not implemented) */


/* external variables */
extern char f_xil;       /* flag external HPIL used */
extern char fserial;     /* flag serial port used */


//extern unsigned _stklen = 8000;     /* stack size */



/* ****************************************** */
/* push_key(int c)                            */
/*                                            */
/* push one character into Emu41 key buffer   */
/* ****************************************** */
void push_key(int c)
{
  if (lgkeybuf<8)
    keybuffer[lgkeybuf++]=c;
}




/* ****************************************** */
/* int traite_display(int force)              */
/*                                            */
/* refresh the display if needed,             */
/* force flag is used to force refresh        */
/* returns true if an update occured          */
/* ****************************************** */
int traite_display(int force)
{
/*  char buf[40];   /* display buffer (37 bytes: 24 lcd + 5 cpu + 8 printer */
  char buf[46];   /* display buffer (45 bytes: 32 lcd + 5 cpu + 8 printer */
  char buf2[40];  /* announ. buffer (36 bytes) */
  static char lastdsp[40]="";    /* last content of display */
  static char lastdsp2[40]="";   /* last content of announ. */
  char fupdate, fupdate2;        /* flags display and announ. to be updated */

  fupdate=0;
  fupdate2=0;

  if (dspon) {
    /* if Emu41 LCD is on, then build the display and announ. buffers */
    display_to_buf(buf);
    ann_to_buf (buf2);
  }
  else {
    /* else blanks the display and announciators */
    /*           12345678901234567890123456789012345678 */
    strcpy(buf ,"                        ");
    strcpy(buf2,"                                    ");
  }

  if (strcmp(buf,lastdsp)!=0) {
    /* if display has changed, it must be updated */
    strcpy(lastdsp,buf);
    fupdate=1;
  }
  if (strcmp(buf2,lastdsp2)!=0) {
    /* if announ. have changed, they must be updated */
    strcpy(lastdsp2,buf2);
    fupdate2=1;
  }

  if (fupdate||force) {
    /* display line update is requested */
    /* appends the CPU state announ. */
    if (!frun) {
      if (speed==1)
        strcat(buf,"    s");
      else if (speed==2)
        strcat(buf,"    f");
      else
        strcat(buf,"     ");
    }
    else {
      if (fslow>0)
        strcat(buf,"    o");
      else
        strcat(buf,"    *");
    }
    /* and appends the printer state (if applicable) */
    switch (mode_printer) {
      case 0: strcat(buf," PRT:MAN"); break;
      case 1: strcat(buf," PRT:NOR"); break;
      case 2: strcat(buf," PRT:TRA"); break;
     default:strcat(buf,"        ");
    }
    /* updates the display line */
    update_display(buf,NULL);
  }

  if (fupdate2) {
    /* updates the HP-41C announ. line */
    update_display(NULL,buf2);
  }

  return(fupdate||fupdate2);
}







/* ****************************************** */
/* int traite_touche(char c)                  */
/*                                            */
/* manages an incoming PC key, and translates */
/* it into a HP-41C key                       */
/* returns:                                   */
/*  -1 if ctrl-Z was pressed (break),         */
/*   0 if no valid key was received,          */
/*  >0 if one valid HP-41C key was detected.  */
/* ****************************************** */
int traite_touche(char c)
{
  int  k;                 /* HP-41C resulting keycode */
  char falpha, fshift;    /* flags HP-41C ALPHA and SHIFT */
  static char arclsto=0;  /* flag for ARCL or ASTO sequence */

  /* table for PC to HP-41C key translation */
  unsigned char tabcode[128]= {
/*  0    1    2    3    4    5    6    7    8    9   10   11   12   13   14   15 */
    0,0xc4,   0,   0,   0,   0,   0,   0,0xc3,   0,0x13,   0,   0,0x13,   0,   0,
/* 16   17   18   19   20   21   22   23   24   25   26   27   28   29   30   31 */
    0,   0,0x87,   0,   0,   0,   0,   0,0x32,   0,   0,   0,   0,   0,   0,   0,
/*       !    "    #    $    %    &    '    (    )    *    +    ,    -    .    / */
 0x37,   0,   0,0x71,0x83,0x31,   0,   0,   0,   0,0x16,0x15,0x77,0x14,0x77,0x17,
/*  0    1    2    3    4    5    6    7    8    9    :    ;    <    =    >    ? */
 0x37,0x36,0x76,0x86,0x35,0x75,0x85,0x34,0x74,0x84,0x17,   0,0x81,0x76,0xc1,0x86,
/*  @    A    B    C    D    E    F    G    H    I    J    K    L    M    N    O */
    0,0x10,0x30,0x70,0x80,0xc0,0x11,0x31,0x71,0x81,0xc1,0x32,0x72,0x82,0x13,0x73,
/*  P    Q    R    S    T    U    V    W    X    Y    Z    [    \    ]    ^    _ */
 0x83,0x14,0x34,0x74,0x84,0x15,0x35,0x75,0x85,0x16,0x36,   0,   0,   0,0x13,   0,
/*  `    a    b    c    d    e    f    g    h    i    j    k    l    m    n    o */
    0,0x10,0x30,0x70,0x80,0xc0,0x11,0x31,0x71,0x81,0xc1,0x32,0x72,0x82,0x13,0x73,
/*  p    q    r    s    t    u    v    w    x    y    z    {    |    }    ~     */
 0x83,0x14,0x34,0x74,0x84,0x15,0x35,0x75,0x85,0x16,0x36,   0,   0,   0,   0,   0
  } ;

/* crtl-X : XEQ
   ctrl-A : ALPHA
   ctrl-R : R/S
   */

  /* auto-shift in alpha mode: test if ALPHA mode and shift not already pressed */
  falpha=  espaceRAM[14*8+1]&0x80;  /* reg. d (flags) */
  fshift=  espaceRAM[14*8+2]&0x1;

  k=0;
  if (c<0) return(0);        /* invalid PC keycode */
  if (c==26) return(-1);     /* ctrl-Z used for execution break */

  /* manages the TAB key */
  if (c==9) {
    /* toggle auto and fast speed */
    if (speed==0) speed=2; else speed=0;
    if (speed) fslow=0; else fslow=VALSLOW;   /* set the slow speed flag */
    traite_display(1);   /* force display update to refresh CPU state announ. */
  }
  else if (c!=0) {
    /* general 1-byte keycode */
    if (!fshift) {
      /* if SHIFT not already active, manages the autoshift */
      if ((c=='%')||(c=='<')||(c=='>')||(c=='#')||(c=='^')||(c=='$'))
        /* auto shift for these particular keycodes */
        push_key(0x12);
      if (falpha) {
        /* in ALPHA mode, autoshift also for these keycodes: */
        if ((c=='+')||(c=='-')||(c=='*')||(c=='/')) push_key(0x12);
        if (!arclsto) {
          /* if not in ASTO or ARCL sequence, autoshift also these codes: */
          if ((c>='0')&&(c<='9')) push_key(0x12); /* shift */
          if ((c=='.')) push_key(0x12);
        }
      }
      else {
        /* not in ALPHA mode. */
        if (c==10) push_key(0x12);   /* ctrl-ENTER is CAT */
      }
      arclsto=0;   /* reset the ARCL/ASTO sequence flag */
    }
    else {
      /* SHIFT is active */
      arclsto=0;
      if (falpha) {
        /* if in ALPHA mode, check the ASTO/ARCL sequence */
        if ((c=='l')||(c=='L')||(c=='m')||(c=='M')) arclsto=1;
      }
    }
    k=tabcode[c];     /* translate the PC keycode to a HP-41C keycode */
  }
  else {
    /* 2-byte keycode (PC control and function keys) */
    c=waitforkey();
    switch (c) {
      case 59: k=0x18; break;  /* F1 : ON */
      case 60: k=0xc6; break;  /* F2 : USER */
      case 61: k=0xc5; break;  /* F3 : PRGM */
      case 62: k=0xc4; break;  /* F4 : ALPHA */
      case 63: k=0x12; break;  /* F5 : SHIFT */
      case 64: k=0xc2; break;  /* F6 : SST */
      case 89: k=0xc2; if (!fshift) push_key(0x12); break;  /* sF6 : BST */
      case 65: k=0x87; break;  /* F7 : R/S */
      case 66: k=0x32; break;  /* F8 : XEQ */
      case 75: k=0xc3; if (!fshift) push_key(0x12); break;  /* <- : CLX */
      case 72: k=0x12; break;  /*  ^ : SHIFT */
      case 77: k=0x11; break;  /* -> : X<>Y */
      case 80: k=0x31; break;  /* v  : RDN */
      case 79: k=0x87; break;  /* END: R/S */
      case 81: k=0xc2; break;  /* PDN: SST */
      case 73: k=0xc2; if (!fshift) push_key(0x12); break;  /*  PUP : BST */
      case 67:
        set_mode_printer();
        return(1);
        break;  /* F9 : MODE */
      case 68:
        set_prx_printer();
        return(1);
        break;  /* F10: PRX */
      case 133-256:
        set_adv_printer();
        return(1);
        break;  /* F11: ADV */
      case 134-256:
        regPC=0;
        break;  /* F12 : reset */
      case 15:  /* shift TAB */
        /* toggle auto and slow speed */
        if (speed==0) speed=1; else speed=0;
        fslow=VALSLOW;
        traite_display(1);
        break;
    }
  }

  /* if got a HP-41C key, push it into Emu41 key buffer */
  if (k) push_key(k);

  return(k);
}




/* ****************************************** */
/* int simulator(void)                        */
/*                                            */
/* Coconut simulator                          */
/*                                            */
/* returns:                                   */
/*  -1 if call to debug (ctrl-z)              */
/*   1 if deep sleep                          */
/*   2 if invalid opcode                      */
/*   3 if break point                         */
/* ****************************************** */
int simulator(void)
{
  int res, k;
  int cpt;
  char c;

  printf("\n");
  prepa_ecran(mode_printer);
  frun=1;        /* CPU is running */

  /* setup slow timer */
  switch (speed) {
    case 0: fslow=VALSLOW; break;
    case 1: fslow=VALSLOW; break;
    case 2: fslow=0; break;
  }

  do {

    if (speed==1) {
      /* slow */
      if (fslowhost)
        res=executeNUT(64);   /* fixed number of cycles */
      else {
        /* on fast host, simulate original spped using delay */
        res=executeNUT(20);  /* 20 opcodes @ 360kHz*56bits = 3 ms*/
        delay(2);
      }
    }
    else if (speed==0) {
      /* auto speed */
      if (lgkeybuf||fserial) {
        /* if a key available, or serial is used, stays in slow speed */
        fserial=0;
        if (fslow==0) {
          fslow=VALSLOW;
          traite_display(1);
        }
        fslow=VALSLOW;
      }
      if (fslow>0) {
        if (fslowhost)
          res=executeNUT(64);
        else {
          res=executeNUT(20);
          delay(2);
        }
        if (fdsp) {
          if (traite_display(0))
            fslow=VALSLOW;
        }
        if (fslow==2) fjmp=0;
        if ((fslow>1)||fjmp)
        fslow--;
        if (fslow==0) {
          traite_display(1);
        }
      }
      else {
        res=executeNUT(256);
        if (fdsp) {
          if (traite_display(0)) {  /* 29/01/04 */
            fslow=VALSLOW;
            traite_display(1);
          }
          fdsp=0;
        }
      }
    }
    else {
      /* (speed==2) high */
      res=executeNUT(256);
    }

    if ((res==2)||(res==3)||(res==4)) {
      /* invalid or break */
      if (res<4) regPC--;
      break;
    }

    if (res==1) {
      /* poweroff */
      frun=0;
      traite_display(1);
      if (Carry) break;  /* deep sleep */
      if (flagKB==0) {
        if (lgkeybuf==0) {
          /* light sleep */
          cursor_off();
          if (f_xil) init_ilext(0,0);   /* HPIL control off */
  	  init_ilext2(0,0);
          k=0;
          cpt=0;

          do {
            c=readkey();
            if (c>=0) {
              k=traite_touche(c);
              if (k<0) {
                fin_ecran();
                return(-1);
              }
            }

            clock();  // remove it?

            if (f_xil) traite_extil();

            if (cpt<50) {
              cpt++;
            }
            else {
              traite_extil2();
              cpt=0;
            }

            send_buffer();        /* printer on LPT1 in background */

          } while (k==0);

          if (f_xil) init_ilext(0,1);   /* HPIL control on */
	  init_ilext2(0,1);
        }
        else {
          /* still keys in buffer */
          flagKey=0;
          dokey();
        }
      } /* endif flagKB */
      frun=1;
      traite_display(1);
      regPC=0;
    }

    else {
      /* real time management */
      if (fdsp) {
        traite_display(0);
        fdsp=0;
      }
      c=readkey();
      if (c>=0) {
        k=traite_touche(c);
        if (k<0) {
          fin_ecran();
          return(-1);
        }
      }
      send_buffer();        /* printer on LPT1 in background */
    }

  } while (1);

  return(res);
}







/* ****************************************** */
/* usage()                                    */
/*                                            */
/* Note: FPU option is not implemented, yet   */
/* ****************************************** */
void usage(void)
{
  printf("Emu41: emulates the HP-41C and HP-IL system\n");
  printf("Syntax:  EMU41 [options]\n");
  printf("Options: /d : debug, call debugger at startup\n");
  printf("         /lm: loop monitor (hpil)\n");
  printf("         /lo: listen only (hpil)\n");
  printf("         /lg: log display write\n");
  printf("         /m : slow host machine (286 or less) \n");
  printf("         /sx: emulation speed (0=auto, 1=normal)\n");
#ifdef FPU
  printf("         /f:  floating point accelerator\n");
#endif
  printf("V2.50 (C) 1997,2009 J-F Garnier   \n\n");
  // added for open source release:
  printf("This program is free software; you can redistribute it and/or\n");
  printf("modify it under the terms of the GNU General Public License\n");
  printf("as published by the Free Software Foundation; either version 2\n");
  printf("of the License, or (at your option) any later version.\n\n");
  printf("This program is distributed in the hope that it will be useful,\n");
  printf("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
  printf("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
  printf("GNU General Public License for more details.\n\n");
  printf("You should have received a copy of the GNU General Public License\n");
  printf("along with this program; if not, write to the Free Software\n");
  printf("Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.\n");

  exit(0);
}



/* ****************************************** */
/* main()                                     */
/*                                            */
/* Emu41 main entry                           */
/*   read and set options                     */
/* ****************************************** */
void main(int argc, char *argv[])
{
  int i, n;
  char *arg;
  int ret;
  int dbg;

  /* read options */
  dbg=0;
  fslowhost=0;
  speed=0;
  fmonit=0;
  ffpu=0;
  for (i=1;i<argc;i++) {
    arg=argv[i];
    if (arg[0]=='/') {
      switch(arg[1]) {
        case '?': usage(); break;
        case 'd':
          init_label("MFENTRY");
          dbg=1;
          break;
        case 'l':
          if (arg[2]=='m') fmonit=1;
          else if (arg[2]=='o') fmonit=2;
          else if (arg[2]=='g') fmonit=3;
          init_log();
          break;
        case 'm':
          fslowhost=1;
          break;
        case 'f':
          ffpu=1;
          break;
        case 's':
          n=atoi(&arg[2]);
          if ((n>=0)&&(n<=2)) speed=n;
          break;
        default:
          fprintf(stderr,"Emu41: unknown option '%c'\n",arg[1]);
          exit(0);
      }
    }
  }

  /* init timer */
  timer_init();

  init_emulate();

#ifdef FPU
  if (ffpu) patch_fpu();
#endif

  if (dbg)
    ret=commande();
  else {
    ret=simulator(); /* ret=1 if PWROFF, -1, 2 or 3 if break */
    if (ret!=1)
      ret=commande(); /* ret=1 if PWROFF, -1 if exit */
  }
  if (ret==1) saveRAM();
  fin_ecran();

  init_ilext2(0, -1);  /* added JFG 21/06/09 for PILBox 1.2 */

}

