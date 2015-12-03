/* ****************************************** */
/* HP-IL device emulation                     */
/* (used in Emu41/Emu71/ILper)                */
/*                                            */
/* ilhdisc.c    LIF drive emulation module    */
/*                                            */
/* Version 2.5, J-F Garnier 1986-2007:        */
/* ****************************************** */

#include <stdio.h>
#include <mem.h>
#include <string.h>


#define HD640K

/* HP-IL data and variables */
#define AID 16     /* Accessory ID = mass storage */
#define DEFADDR 2  /* default address after AAU */
static char did[]="HDRIVE1";  /* Device ID */
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

/* disk management variables  */
static int devl, devt;    /* dev lstn & tlker */
static int oc;            /* byte pointer */
static int pe;            /* record pointer */
static int pe0;
static int fpt;           /* flag pointer */
static int flpwr;         /* flag partial write */
static int ptout;         /* pointer out */
static FILE *fic;         /* file handler */
#ifdef HD640K
/* 640kb floppy drive descriptor */
static char lif[] = {0,0, 0,80, 0,0, 0,2, 0,0, 0,16 } ; /* 80 tracks, 2 sides, 16 sect/track */
static int nbe = 2560;  /* number of 256-byte sectors */
#else
/* 128Kb cassette drive descriptor */
static char lif[] = {0,0, 0,2, 0,0, 0,1, 0,0, 1,0 } ; /* 2 tracks, 1 side, 256 sect/track */
static int nbe = 512;   /* number of 256-byte sectors */
#endif
static char buf0[256];  /* buffer 0 */
static char buf1[256];  /* buffer 1 */
static char hdiscfile[80];  /* path to disk image file */


/* ***************************************************************** */
/* hardware access functions                                         */
/* ***************************************************************** */


/* read one sector n° pe (256 bytes) into buf0 */
static void rrec(void)
{
  int n;

  fic=fopen(hdiscfile,"rb");
  if (fic==NULL)
    status=20;
  else {
    n=fseek(fic,pe*256L,0);
    if (n!=0)
      memset(buf0,0xff,256);
    else {
      n=fread(buf0,1,256,fic);
      if (n!=256)
        memset(buf0,0xff,256);
    }
    fclose(fic);
  }

}

/* write buf0 to one sector n° pe (256 bytes) */
static void wrec(void)
{
  int n;

  fic=fopen(hdiscfile,"r+b");
  if (fic==NULL)
    status=29;
  else {
    n=fseek(fic,pe*256L,0);
    if (n!=0)
      status=24;
    else {
      n=fwrite(buf0,1,256,fic);
      if (n!=256) status=29;
    }
    fclose(fic);
  }
}

/* "format" a LIF image file */
static void format(void)
{
  char buf[256];
  int i, n;

  status=0;
  fic=fopen(hdiscfile,"r+b");
  if (fic==NULL) {
    fic=fopen(hdiscfile,"w+b");
  }

  if (fic!=NULL) {
    memset(buf,0xff,256);
    fseek(fic,0,0);
    for (i=0;i<128;i++) {
      n=fwrite(buf,1,256,fic);
      if (n!=256) { status=29; break; }
    }
    fclose(fic);
  }
  else
    status=29;
}





/* ***************************************************************** */
/* hardware independant functions                                    */
/* ***************************************************************** */


#include "ildrive.c"




/* ***************************************************************** */
/* exported functions                                                */
/* ***************************************************************** */

void init_ilhdisc(char *fic)
{
  if (fic[0]==0) strcpy(hdiscfile,"hdrive1.dat"); else strcpy(hdiscfile,fic);
  status=0;
  adr=0;
  ptsdi=0;
  fetat=0;

  clrdrv();
}




int ilhdisc(int frame)
{
  if ((frame&0x400)==0) frame=traite_doe(frame);
  else if ((frame&0x700)==0x400) frame=traite_cmd(frame);
  else if ((frame&0x700)==0x500) frame=traite_rdy(frame);

  return(frame);
}


