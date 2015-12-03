/* ****************************************** */
/* HP-IL device emulation                     */
/* (used in Emu41/Emu71/ILper)                */
/*                                            */
/* ildos.c   HP-IL DOS link module            */
/*                                            */
/* Version 2.5, J-F Garnier 1997-2007:        */
/* ****************************************** */

#include <stdio.h>
#include <conio.h>




/* HP-IL data and variables */
#define AID 78     /* Accessory ID = general interface */
#define DEFADDR 29 /* default address after AAU */
static char did[]="DOSLINK";  /* Device ID */
static int status; /* HP-IL status (always 0 here)*/
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

static FILE *ficin = NULL;
static FILE *ficout = NULL;

/* file names for Emu41: */
#define FILEIN  "EMU41IN.DAT"
#define FILEOUT "EMU41OUT.DAT"


/* ****************************************** */
/* init_ildos()                               */
/*                                            */
/* init the virtual HPIL DOS link device      */
/* ****************************************** */
void init_ildos(void)
{
  status=0;
  adr=0;
  fetat=0;
  ptsdi=0;


}


/* ****************************************** */
/* indos(n)                                   */
/*                                            */
/* write an incoming character to the DOS file*/
/* ****************************************** */
static void indos(int n)
{
  if (ficout==NULL)
    ficout=fopen(FILEOUT,"wb");
  if (ficout!=NULL) {
    fputc(n,ficout);
  }
}

/* ****************************************** */
/* outdos()                                   */
/*                                            */
/* returns a character read from the DOS file */
/* ****************************************** */
static int outdos(void)
{
  int frame;

  if (ficin==NULL)
    ficin=fopen(FILEIN,"rb");
  if (ficin!=NULL)
    frame=fgetc(ficin);
  else
    frame=-1;
  if (frame<0) frame=0x540;  /* EOT */
  return(frame);
}

/* ****************************************** */
/* clrdos()                                   */
/*                                            */
/* close the DOS input and output files       */
/* ****************************************** */
static void clrdos(void)
{
  if (ficin!=NULL) {
    fclose(ficin);
    ficin=NULL;
  }
  if (ficout!=NULL) {
    fclose(ficout);
    ficout=NULL;
  }
}


/* ****************************************** */
/* traite_doe(frame)                          */
/*                                            */
/* manage the HPIL data frames                */
/* returns the returned frame                 */
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
          frame=outdos();
      }
      else {
        /* end of SST, SAI */
        frame=0x540;  /* EOT */
        fetat=0x40;
      }
    }
    else {
      /* listener */
      indos(frame);
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
          if (fetat&0x80) clrdos();
          break;
        case 20: /* DCL */
          clrdos();
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
        fetat &= 0xfd;    /* abort transfer */
      }
      else if (n==96) { /* SDA */
        frame=outdos();
        fetat=0xc2;   /* active talker (data) */
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



/* ****************************************** */
/* ildos(frame)                               */
/*                                            */
/* manage a HPIL frame                        */
/* returns the returned frame                 */
/* ****************************************** */
int ildos(int frame)
{
  if ((frame&0x400)==0) frame=traite_doe(frame);
  else if ((frame&0x700)==0x400) frame=traite_cmd(frame);
  else if ((frame&0x700)==0x500) frame=traite_rdy(frame);

  return(frame);
}


