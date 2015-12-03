/* ****************************************** */
/* HP-IL device emulation                     */
/* (used in Emu41/Emu71/ILper)                */
/*                                            */
/* ildrive.c   Disk drive module              */
/*   disc drive management functions,         */
/*   hardware independant                     */
/*                                            */
/* Version 2.5, J-F Garnier 1986-2007:        */
/*                                            */
/* Based on previous personal work:           */
/* 1986: ILPER4 video+disc, (6502 assembler)  */
/* 1988: ILPER5 discil module                 */
/* 1993: ILPER ported on PC (8086 assembler)  */
/* 1997: rewrote in C and included in Emu41   */
/* ****************************************** */

/* ****************************************** */
/* clrdrv()                                   */
/*                                            */
/* Clear drive, reset internal pointers       */
/* ****************************************** */
static void clrdrv(void)
{
  fpt=0;  /* reset DDL4 partial sequence */
  pe=0;   /* record pointer = 0 */
  oc=0;   /* byte pointer = 0 */
}


/* ****************************************** */
/* indata(n)                                  */
/*                                            */
/* receive data to disc according to DDL cmd  */
/* ****************************************** */
static void indata(int n)
{
  switch (devl) {
    case 0:
    case 2:     /* JFG 30/11/03 */
    case 6:
      buf0[oc]=n;
      oc++;
      if (oc>255) {
        oc=0;
        wrec();
        pe++;
        if (flpwr) rrec();
      }
      else {
        if (n&0x200) {
          /* END */
          wrec();
          if (!flpwr) pe++;
        }
      }
      break;
    case 1:
      buf1[oc]=n;
      oc++;
      if (oc>255)
        oc=0;
      break;
    case 3:
      oc=n&255;
      break;
    case 4:
      if (fpt) {
        pe0 &= 0xff00;
        pe0 |= n & 255;
        if (pe0<nbe) {
          pe=pe0;
          status=0;
        }
        else {
          status=28;
        }
        fpt=0;
      }
      else {
        pe0 &= 255;
        pe0 |= n << 8;
        fpt--;
      }

  }
}

/* ****************************************** */
/* outdta()                                   */
/*                                            */
/* send data from disc according to DDT cmd   */
/* ****************************************** */
static int outdta(void)
{
  int frame;

  switch (devt) {
    case 0:
    case 2:
      frame=buf0[oc]&255;
      oc++;
      if (oc>255) {
        oc=0;
        rrec();
        pe++;
      }
      break;
    case 1:
      frame=buf1[oc]&255;
      oc++;
      if (oc>255) {
        oc=0;
        devt=15;
      }
      break;
    case 3:
      switch (ptout) {
        case 0: frame= pe >> 8; break;
        case 1: frame= pe & 255; break;
        case 2: frame= oc & 255; break;
        default: frame= 0x540; /* EOT */
      }
      ptout++;
      break;
    case 6:
      if (ptout<12) {
        frame=lif[ptout];
        ptout++;
      }
      else {
        frame= 0x540; /* EOT */
      }
      break;
    case 7:
      switch (ptout) {
        case 0: frame= nbe >> 8; break;
        case 1: frame= nbe & 255; break;
        default: frame= 0x540; /* EOT */
      }
      ptout++;
      break;
    default: frame= 0x540; /* EOT */
  }
  if (frame==0x540)  /* EOT */
    fetat=0x40;   /* JFG 30/11/03 */
  return(frame);
}



/* ****************************************** */
/* traite_doe(frame)                          */
/*                                            */
/* handle the data frames                     */
/* ****************************************** */
static int traite_doe(int frame)
{
  char c;

  if (fetat&0x80) {
    /* addressed */
    if (fetat&0x40) {
      /* talker */
      if (fetat&2) {
        /* data (SDA) or accessory ID (SDI) */
        if (fetat&1) {
          /* SDI */
          if (ptsdi) {
            c=did[ptsdi];
            if (c==0)
              ptsdi=0;
            else {
              ptsdi++;
              frame=c;
            }
          }
          if (ptsdi==0) {
            frame=0x540; /* EOT */
            fetat=0x40;
          }
        }
        else
          /* SDA */
          frame=outdta();
      }
      else {
        /* end of SST, SAI */
        frame=0x540;  /* EOT */
        fetat=0x40;
      }
    }
    else {
      /* listener */
      indata(frame);
    }
  }
  return(frame);
}

/* ****************************************** */
/* copybuf()                                  */
/*                                            */
/* copy buffer 0 to buffer 1                  */
/* ****************************************** */
static void copybuf(void)
{
  oc=0;
  memcpy(buf1,buf0,256);
}

/* ****************************************** */
/* exchbuf()                                  */
/*                                            */
/* exchange buffers 0 and 1                   */
/* ****************************************** */
static void exchbuf(void)
{
  char buf2[256];

  oc=0;
  memcpy(buf2,buf1,256);
  memcpy(buf1,buf0,256);
  memcpy(buf0,buf2,256);
}

/* ****************************************** */
/* traite_cmd(frame)                          */
/*                                            */
/* handle command frames                      */
/* ****************************************** */
static int traite_cmd(int frame)
{
  int n;

  n=frame & 255;
  switch (n>>5) {
    case 0:
      switch (n) {
        case 4:  /*SDC */
          if (fetat&0x80) clrdrv();
          break;
        case 20: /* DCL */
          clrdrv();
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
      break;
    case 5:  /* DDL */
      n &= 31;
      if ((fetat&0xc0)==0x80) {
        devl=n;
        switch (n) {
          case 1: flpwr=0; break;
          case 2: oc=0; flpwr=0; break;
          case 4: flpwr=0; fpt=0; break;  /* 01/02/03 */
          case 5: format(); break;
          case 6: flpwr=0x80; rrec(); break;
          case 7: fpt=0; pe=0; oc=0; break;
          case 8: wrec(); if (flpwr==0) pe++; break;
          case 9: copybuf(); break;
          case 10:exchbuf(); break;
        }
      }
      break;
    case 6:  /* DDT */
      n &= 31;
      if ((fetat&0x40)==0x40) {
        devt=n;
        ptout=0;
        switch (n) {
          case 0: flpwr=0; break;
          case 2: rrec(); oc=0; flpwr=0; pe++; break;
          case 4: exchbuf(); break;
        }
      }
      break;
  }


  return(frame);
}



/* ****************************************** */
/* traite_rdy(frame)                          */
/*                                            */
/* handle the ready frames                    */
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
        fetat=0xc2;   /* moved before outdta JFG 30/11/03 */
        frame=outdta();
      }
      else if (n==97) { /* SST */
        frame=status;
        status &= 0xe0;
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

