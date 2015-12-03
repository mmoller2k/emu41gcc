/* ****************************************** */
/* HP-IL device emulation                     */
/* (used in Emu41/Emu71/ILper)                */
/*                                            */
/* ilserial.c         serial port module      */
/*   low level serial routines (BIOS calls)   */
/*                                            */
/* Version 2.5, J-F Garnier 2005-2007:        */
/* ****************************************** */

static int comport;

/* HP-IL data and variables */
#define AID 65     /* Accessory ID = interface */
#define DEFADDR 4  /* default address after AAU */
static int status; /* HP-IL status */
static int adr;    /* bits 0-5 = HP-IL address, bit 7 = 1 means auto address taken */
static int fetat; /* HP-IL state machine flags: 
/* bit 7, bit 6:
   0      0       idle
   1      0       addressed listen
   0      1       addressed talker 
   1      1       active talker
bit 1: SDA, SDI
bit 0: SST, SDI, SAI 
   */
static int ptsdi;  /* output pointer for device ID */

static frdy;

/* static char buf[2048]; */
static int ptrin, ptrout;


/* ****************************************** */
/* init_ilserial(d, config)                   */
/*                                            */
/* init a virtual HPIL serial device          */
/* config  port p                             */
/*  use BIOS calls (INT 14)                   */
/* ****************************************** */
static void init_ilserial(int d, int config)
{
  status=0;
  adr=0;
  fetat=0;
  ptsdi=0;

  comport=d;

  if (config!=0) {
    _DX = comport;
    _AL = config;
    _AH = 0;
    geninterrupt(0x14);
  }

  ptrin=0;
  ptrout=0;

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
  _DX = comport;
  _AH = 3;                     /* status */
  geninterrupt(0x14);
  return(_AH);
}

#if 0
/* serial buffers are not implemented, upto now */

static int send_buffer_serial(void)
{
  int st, n;

  n=ptrin-ptrout;
  if (n<0) n+=2048;
  if (n!=0) {
    st=status_serial();
    if ((st&0x60)==0x60) {
      do {
        n=ptrin-ptrout;
        if (n<0) n+=2048;
        if (n==0) break;
        st=send_car(buf[ptrout]);
        if ((st&0x60)!=0x60) break;
        ptrout++;
        if (ptrout>=2048) ptrout=0;
        delay(1);
      } while (1);
    }
  }
  return(n);
}


static void add_buffer(int c)
{
  int i, n;

  i=0;
  do {
    send_buffer_serial();
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
  send_buffer_serial();

}

#endif

static void clr_buffer(void)
{
  ptrin=0;
  ptrout=0;
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

  if (fetat&0x80) {
    /* addressed */
    if (fetat&0x40) {
      /* talker */
      if (fetat&2) {
        /* data (SDA) or accessory ID (SDI) */
        if (fetat&1) {
          /* SDI */
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
          /* SDA */
/*          frame=outserial(); */
          do {
            n=read_car();
            if (n<0)
              if (tstkey()) frdy=0;
          } while ((n<0)&&frdy);
          if (n<0) {
            frame=0x540;  /* EOT */
            fetat=0x40;
          }
          else
            frame=n;
        }
      }
      else {
        /* end of SST, SAI */
        frame=0x540;  /* EOT */
        fetat=0x40;
      }

    }

    else {
      /* listener */
      c=frame & 255;
/*      add_buffer(c); */
      do {
        n=send_car(c);
        if (n&0x80)
          if (tstkey()) frdy=0;
      } while ((n&0x80)&&frdy);
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
        if (n==(adr&31)) {
          fetat=0x80;
          frdy=1;
        }
      }
      break;
    case 2:    /* TAD */
      n &= 31;
      if (n==(adr&31)) {
        /* if MTA go to talker state */
        fetat=0x40;
        frdy=1;
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
        fetat &= 0xfd;    /* abort transfer */
      }
      else if (n==96) { /* SDA */
        frame=read_car();
        if (frame<0)
          frame=0x540;  /* EOT */
        else
          fetat=0xc2;   /* active talker (data) */
      }
      else if (n==97) { /* SST */
        status=status_serial();
        frame=status;
        fetat=0xc1;   /* active talker (single byte status) */
      }
      else if (n==98) { /* SDI */
        frame=did[0];
        ptsdi=1;
        fetat=0xc3;   /* active talker (multiple byte status) */
      }
      else if (n==99) { /* SAI */
        frame=AID;
        fetat=0xc1;   /* active talker (single byte status) */
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

