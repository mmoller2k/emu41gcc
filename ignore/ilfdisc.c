/* ****************************************** */
/* HP-IL device emulation                     */
/* (used in Emu41/Emu71/ILper)                */
/*                                            */
/* ilfdisc.c    LIF drive emulation module    */
/*                                            */
/* Version 2.5, J-F Garnier 1986-2007:        */
/* ****************************************** */

#include <stdio.h>
#include <mem.h>
#include <dos.h>
#include <bios.h>

#if __TURBOC__ < 0x200
void _fmemcpy(int far *dest, int far *src, int n);
#endif

/* HP-IL data and variables */
#define AID 16     /* Accessory ID = mass storage */
#define DEFADDR 2  /* default address after AAU */
static char did[]="FDRIVE1";  /* Device ID */
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
/* 640kb floppy drive descriptor */
static char lif[] = {0,0, 0,80, 0,0, 0,2, 0,0, 0,16 } ; /* 80 tracks, 2 sides, 16 sect/track */
static int nbe = 2560;  /* number of 256-byte sectors */
static char buf0[256];  /* buffer 0 */
static char buf1[256];  /* buffer 1 */
static int drive;       /* 0=drive A, 1=drive B */


/* ***************************************************************** */
/* hardware access functions, use BIOS calls                         */
/* (WARNING: these functions work on DOS and up to Windows 98        */
/*  but DON'T WORK on Windows NT/ME/2000/XP and later OS             */
/* ***************************************************************** */


/* read one sector n° pe (256 bytes) into buf0 */
static void rrec(void)
{
  int n;
  void far *ancint;
  char new_param[16];
  void interrupt (*ptr) ();

  /* use the INT 1E hook to change the diskette parameter table */
  ancint=getvect(0x1e);
  _fmemcpy(new_param,ancint,16);
  new_param[3]=1;  /* 256 bytes/sector */
  ptr=new_param;
  setvect(0x1e,ptr);


/*  biosdisk(int cmd, int drive, int head, int track, int sector, int nsects, void *buffer); */
  n=biosdisk(2, drive, (pe>>4)&1,(pe>>5), (pe&15)+1, 1, buf0);
  if (n==6)
    n=biosdisk(2, drive, (pe>>4)&1,(pe>>5), (pe&15)+1, 1, buf0);

  switch (n) {
    case 0:    status=0; break;
    case 6:    status=23; break;  /* new disk */
    case 0x80: status=20; break;  /* no disk */
    default:   status=24;
  }

  /* restore the previous disquette parameter table */
  ptr=ancint;
  setvect(0x1e,ptr);

}


/* write buf0 to one sector n° pe (256 bytes) */
static void wrec(void)
{
  int n;
  void far *ancint;
  char new_param[16];
  void interrupt (*ptr) ();

  /* use the INT 1E hook to change the diskette parameter table */
  ancint=getvect(0x1e);
  _fmemcpy(new_param,ancint,16);
  new_param[3]=1;  /* 256 bytes/secteur */
  ptr=new_param;
  setvect(0x1e,ptr);


/*  biosdisk(int cmd, int drive, int head, int track, int sector, int nsects, void *buffer); */
  n=biosdisk(3, drive, (pe>>4)&1,(pe>>5), (pe&15)+1, 1, buf0);

  switch (n) {
    case 0:    status=0; break;
    case 0x80: status=20; break;
    default:   status=24;
  }

  /* restore the previous disquette parameter table */
  ptr=ancint;
  setvect(0x1e,ptr);

}

/* disquette formatting is not supported */
static void format(void)
{
  status=24;
}





/* ***************************************************************** */
/* hardware independant functions                                    */
/* ***************************************************************** */


#include "ildrive.c"




/* ***************************************************************** */
/* exported functions                                                */
/* ***************************************************************** */

void init_ilfdisc(int d)
{
  drive=d;
  status=0;
  adr=0;
  ptsdi=0;
  fetat=0;

  clrdrv();
}




int ilfdisc(int frame)
{
  if ((frame&0x400)==0) frame=traite_doe(frame);
  else if ((frame&0x700)==0x400) frame=traite_cmd(frame);
  else if ((frame&0x700)==0x500) frame=traite_rdy(frame);

  return(frame);
}

