/* ****************************************** */
/* Emu41: HP-41C software emulator for PC     */
/*                                            */
/* printer.c   82143 printer simulator module */
/*                                            */
/* Version 2.5, J-F Garnier 1997-2007         */
/* ****************************************** */

/* Conditional compilation:
  none
*/

#include <ctype.h>

/* commun variables to all modules */
#define GLOBAL extern
#include "nutcpu.h"



/* ****************************************** */
/* print_char(int n)                          */
/*                                            */
/* send one character to the HP82143 buffer   */
/* actual printing to the screen emulating    */
/* the HP82143 is done by print_str(buffer)   */
/* ****************************************** */
void print_char(int n)
{
  static char buffer[82];     /* line buffer */
  static int buflen=0;        /* line len */
  static int flow=0;          /* flag lowercase */
  static int fdwid=0;         /* flag double width */
  int i, j;

  n &= 255;                   /* security */

  if (n>127) {
    /* control char. */
    switch (n) {
    case 224:
      /* print to left if buffer NOT empty */
      if (buflen) {
        buffer[buflen++]=13;
        buffer[buflen++]=10;
        buffer[buflen]=0;
        print_str(buffer);    /* print buffer on display device */
        buflen=0;             /* and clear buffer */
        /* reset flags */
        flow=0;
        fdwid=0;
      }
      break;
    case 232:
      /* print to right */
      if (buflen>31) buflen=31;
      /* insert spaces into head */
      for (i=(buflen-1),j=30;i>=0;i--,j--)
        buffer[j]=buffer[i];
      for (;j>=0;j--)
        buffer[j]=' ';
      buflen=31;
      buffer[buflen++]=13;
      buffer[buflen++]=10;
      buffer[buflen]=0;
      print_str(buffer);
      buflen=0;
      /* reset flags */
      flow=0;
      fdwid=0;
      break;
    case 209:
      /* lowercase */
      flow=1;
      break;
    case 212:
      /* double width */
      fdwid=1;
      break;
    case 213:
      /* lower case & double width */
      flow=1;
      fdwid=1;
      break;
    case 255:
      /* sometime sent after ADV, don't know why, ignore it */
      break;
    default:
      if ((n>160)&&(n<192)) {
        /* accumulate spaces */
        for (i=160;i<n;i++) {
          if (buflen<70) {
            buffer[buflen++]=' ';
            if (fdwid)
              buffer[buflen++]=' ';
          }
        }
      }
      else {
        /* unknown code: print debug info */
        if (buflen<70) {
          buffer[buflen++]='\\';
          buffer[buflen++]='0'+(n/100);
          buffer[buflen++]='0'+((n/10)%10);
          buffer[buflen++]='0'+(n%10);
        }
      }
    } /* endswitch */
  }

  else {
    /* normal char. */
    if (flow)
      n=tolower(n);
    /* convert 41 special char to ASCII */
    switch (n) {
      case   0: n='*'; break;
      case  12: n='u'; break;
      case  29: n='#'; break;
      case 124: n='a'; break;  /* angle */
      case 126: n='s'; break;  /* sigma */
      case 127: n='`'; break;  /* append sign */
      default:
       if ((n<32)||(n>127))  n='.';
    }
    if (buflen<70) {
      buffer[buflen++]=n;
      if (fdwid)
        buffer[buflen++]=' ';
    }
  }
}





/* ****************************************** */
/* set_mode_printer()                         */
/*                                            */
/* set the HP82143 printer mode:              */
/*   0:MAN -> 1:NOR -> 2:TRA -> 3:OFF         */
/*  -1 means not plugged                      */
/* ****************************************** */
void set_mode_printer(void)
{
  if (mode_printer>=0) {
    mode_printer++;
    if (mode_printer>3) mode_printer=0;
    flagPrter=1;
  }
}

/* ****************************************** */
/* set_prx_printer()                          */
/*                                            */
/* set the flag for pressed PRINT key         */
/* ****************************************** */
void set_prx_printer(void)
{
  if ((mode_printer>=0)&&(mode_printer<=2)) {
    flagPrx=4;
    flagPrter=1;
  }
}

/* ****************************************** */
/* set_adv_printer()                          */
/*                                            */
/* set the flag for pressed ADVANCE key       */
/* ****************************************** */
void set_adv_printer(void)
{
  if ((mode_printer>=0)&&(mode_printer<=2)) {
    flagAdv=4;
    flagPrter=1;
  }
}

/* ****************************************** */
/* get_printer_status(char *regC)             */
/*                                            */
/* get the status word from the HP82143       */
/* ****************************************** */
int get_printer_status(void)
{
  int n;

  n=0;
  if ((mode_printer>=0)&&(mode_printer<=2)) {
    /* printer is on, return status */
    n=mode_printer<<14;
    if (flagPrx) n |= 1<<13;
    if (flagAdv) n |= 1<<12;
    n |= 3<<8;  /* idle, buffer empty */
    n |= 3;     /* I don't remember why ... */
    if (flagPrx) flagPrx--;
    if (flagAdv) flagAdv--;
    flagPrter=0;
  }
  return(n);
}


/* ****************************************** */
/* test_printer_flag(int n)                   */
/*                                            */
/* test a 82143 flag                          */
/* ****************************************** */
int test_printer_flag(int n)
{
  int f;
    
  if ((mode_printer>=0)&&(mode_printer<=2)) {
    /* printer is on, return flag */
    switch (n) {
      case 0: f=0; break;   /* BUSY? */
      case 1: f=1; break;   /* POWON? */
      case 2: f=flagPrter; f=1; break;  /* data to be read (forced to 1) */
      /* (this is not consistent with the NPIC description, but it works...) */
      default: f=0;
    }
  }
  else
    f=0;    
        
  return(f);
}

