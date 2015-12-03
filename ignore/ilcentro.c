/* ****************************************** */
/* HP-IL device emulation                     */
/* (used in Emu41/Emu71/ILper)                */
/*                                            */
/* ilcentro.c   HP-IL parallel printer module */
/*                                            */
/* Version 2.5, J-F Garnier 1997-2007:        */
/* ****************************************** */

#include <stdio.h>
#include <conio.h>
#include <dos.h>



/* HP-IL data and variables */
#define AID 64     /* Accessory ID = interface */
#define DEFADDR 4  /* default address after AAU */
static char did[]="LPRTER1";   /* Device ID */
static int status; /* HP-IL status (LPT1 port BIOS status here)*/
static int adr;    /* bits 0-5 = HP-IL address, bit 7 = 1 means auto address taken */
static int fetat;  /* HP-IL state machine flags:
/* bit 7, bit 6:
   0      0       idle
   1      0       addressed listen
   0      1       addressed talker
   1      1       active talker */
static int ptsdi;  /* output pointer for device ID */

extern int fmonit;   /* monitor flag */

static FILE *fic = NULL;

static char buf[2048];     /* printer buffer */
static int ptrin, ptrout;  /* input and output pointers */

static int fdos=0;    /* DOS mode, JFG 02/01/2009 */



/* ****************************************** */
/* init_ilcentro()                            */
/*                                            */
/* init the virtual HPIL centronic device     */
/* ****************************************** */
void init_ilcentro(int d)
{
  fdos=d;    /* DOS mode, JFG 02/01/2009 */
  status=0;
  adr=0;
  fetat=0;
  ptsdi=0;

  if (fmonit==3)
    fic=fopen("printer.log","wt");
  else
    fic=NULL;

  ptrin=0;
  ptrout=0;

}


/* ****************************************** */
/* send_car(car)                              */
/*                                            */
/* send a character to the printer interface  */
/*   (use BIOS call)                          */
/* ****************************************** */
static int send_car(char car)
{
  if (fdos==0) {
    _AH = 0;
    _AL = car;
    _DX = 0;
    geninterrupt(0x17);
    return(_AH);
  }
  else {
    bdos(0x5,car,0);
    return(0);
  }
}

/* ****************************************** */
/* status_centro()                            */
/*                                            */
/* read the status of the printer interface   */
/*   (use BIOS call)                          */
/* ****************************************** */
static int status_centro(void)
{
  if (fdos==0) {
    _AH = 2;                     /* status */
    _DX = 0;                     /* LPT1 */
    geninterrupt(0x17);
    return(_AH);
  }
  else
    return(0x10);
}

/* ****************************************** */
/* send_buffer()                              */
/*                                            */
/* send characters from the buffer            */
/*   to the printer                           */
/* ****************************************** */
int send_buffer(void)
{
  int st, n;

  n=ptrin-ptrout;
  if (n<0) n+=2048;
  if (n!=0) {
    st=status_centro();
    if ((st&0x39)==0x10) {
      do {
        n=ptrin-ptrout;
        if (n<0) n+=2048;
        if (n==0) break;
        st=send_car(buf[ptrout]);
        if ((st&0x39)!=0x10) break;
        ptrout++;
        if (ptrout>=2048) ptrout=0;
        delay(1);
      } while (1);
    }
  }
  return(n);
}

/* ****************************************** */
/* clr_buffer(c)                              */
/*                                            */
/* clear the printer buffer                   */
/* ****************************************** */
static void clr_buffer(void)
{
  ptrin=0;
  ptrout=0;
}

/* ****************************************** */
/* add_buffer(c)                              */
/*                                            */
/* add a character to the printer buffer      */
/* ****************************************** */
static void add_buffer(int c)
{
  int i, n;

  i=0;
  do {
    send_buffer();
    n=ptrin-ptrout;
    if (n<0) n += 2048;
    if (n<2047) break;
    delay(100);
    i++;
    if (i==10) break;
  } while (1);
  if (n<2047) {
    buf[ptrin]=c;
    ptrin++;
    if (ptrin==2048) ptrin =0;
  }
  send_buffer();

}

/* ****************************************** */
/* traite_doe(frame)                          */
/*                                            */
/* manage the HPIL data frames                */
/* returns the returned frame                 */
/* ****************************************** */
static int traite_doe(int frame)
{
  int n;
  char c;

  n=frame & 255;
  if (fetat&0x80) {
    /* addressed */
    if (fetat&0x40) {
      /* talker */
      if (ptsdi!=0) {
        c=did[ptsdi];
        frame=c;
        if (c==0) ptsdi=0; else ptsdi++;
      }
      if (ptsdi==0) {
        frame=0x540;  /* EOT */
        fetat=0x40;
      }
    }
    else {
      /* listener */
      add_buffer(n);
      if ((fic!=NULL)&&(n!=13)) fprintf(fic,"%c",n);
    }
  }
  return(frame);
}

/* ****************************************** */
/* traite_cmd(frame)                          */
/*                                            */
/* manage the HPIL command frames             */
/* returns the returned frame                 */
/* ****************************************** */
static int traite_cmd(int frame)
{
  int n;

  n=frame & 255;
  switch (n>>5) {
    case 0:
      switch (n) {
        case 4:  /*SDC */
          if (fetat&0x80) clr_buffer();
          break;
        case 20: /* DCL */
          clr_buffer();
          break;
      }
      break;
    case 1:     /* LAD */
      n &= 31;
      if (n==31) {
        /* UNL, if not talker then go to idle state */
        if ((fetat&0x40)==0) fetat=0;
      }
      else {
        /* else, if MLA go to listen state */
        if (n==(adr&31)) fetat=0x80;
      }
      break;
    case 2:    /* TAD */
      n &= 31;
      if (n==(adr&31)) {
        /* if MTA go to talker state */
        fetat=0x40;
      }
      else {
        /* else if addressed talker, go to idle state */
        if ((fetat&0x40)!=0) fetat=0;
      }
      break;
    case 4:
      n &= 31;
      switch (n) {
        case 16:  /* IFC */
          fetat=0;
          break;
        case 26:   /* AAU */
          adr=DEFADDR;
          break;
      }
  }


  return(frame);
}



/* ****************************************** */
/* traite_rdy(frame)                          */
/*                                            */
/* manage the HPIL ready frames               */
/* returns the returned frame                 */
/* ****************************************** */
static int traite_rdy(int frame)
{
  int n;

  n=frame & 255;
  if (n<=127) {
    /* sot */
    if (fetat&0x40) {
      /* if addressed talker */
      if (n==66) {   /* NRD */
        ptsdi=0;     /* abort transfer */
      }
      else if (n==96) { /* SDA */
        /* no response */
      }
      else if (n==97) { /* SST */
        status=status_centro();
        frame=status;
        fetat=0xc0;  /* active talker */
      }
      else if (n==98) { /* SDI */
        frame=did[0];
        ptsdi=1;
        fetat=0xc0;
      }
      else if (n==99) { /* SAI */
        frame=AID;
        fetat=0xc0;
      }
    }
  }
  else if (n<0x9e) {
    /* AAD, if not already an assigned address, take it */
    if ((adr&0x80)==0) {
      adr=n;
      frame++;
    }
  }

  return(frame);
}



/* ****************************************** */
/* ilcentro(frame)                            */
/*                                            */
/* manage a HPIL frame                        */
/* returns the returned frame                 */
/* ****************************************** */
int ilcentro(int frame)
{
  if ((frame&0x400)==0) frame=traite_doe(frame);
  else if ((frame&0x700)==0x400) frame=traite_cmd(frame);
  else if ((frame&0x700)==0x500) frame=traite_rdy(frame);

  return(frame);
}

