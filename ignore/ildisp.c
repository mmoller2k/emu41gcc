/* ****************************************** */
/* HP-IL device emulation                     */
/* (used in Emu41/Emu71/ILper)                */
/*                                            */
/* ildisp.c   HP-IL display module            */
/*                                            */
/* Version 2.5, J-F Garnier 1986-2007:        */
/*                                            */
/* Based on previous personal work:           */
/* 1986: ILPER4 video+disc, (6502 assembler)  */
/* 1988: ILPER5 videoil module                */
/* 1993: ILPER ported on PC (8086 assembler)  */
/* 1997: rewriten in C and included in Emu41  */
/* ****************************************** */

#include <stdio.h>
#include <conio.h>



/* HP-IL data and variables */
#define AID 48     /* Accessory ID = video */
#define DEFADDR 3  /* default address after AAU */
static char did[]="DISPLAY";   /* Device ID */
static int status; /* HP-IL status (always 0 here)*/
static int adr;    /* bits 0-5 = HP-IL address, bit 7 = 1 means auto address taken */
static int fvideo; /* HP-IL state machine flags: 
/* bit 7, bit 6:
   0      0       idle
   1      0       addressed listen
   0      1       addressed talker 
   1      1       active talker
( this choice comes from my original 6502 assembly code [PLTERx and ILPERx applications, 1984-1988]
  that used the efficient BIT opcode to test both bit 6 and 7)
*/
static int ptsdi;  /* output pointer for device ID */

extern int fmonit;   /* monitor flag */

static FILE *fic = NULL;


/* ****************************************** */
/* init_log()                                 */
/*                                            */
/* init the display log file                  */
/* ****************************************** */
void init_log(void)
{
  if (fmonit)
    fic=fopen("disp.log","wt");
  else
    fic=NULL;
}


/* ****************************************** */
/* print_str()                                */
/*                                            */
/* print a string to the virtual HPIL display */
/* ****************************************** */
void print_str(char *s)
{
  disp_string(s);
  if (fic!=NULL)
    fprintf(fic,"%s",s);
}


/* ****************************************** */
/* init_ildisplay()                           */
/*                                            */
/* init the virtual HPIL display device       */
/* ****************************************** */
void init_ildisplay(void)
{
  status=0;
  adr=0;
  fvideo=0;
  ptsdi=0;
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
  if (fvideo&0x80) {
    /* addressed */
    if (fvideo&0x40) {
      /* talker */
      if (ptsdi!=0) {
        c=did[ptsdi];
        frame=c;
        if (c==0) ptsdi=0; else ptsdi++;
      }
      if (ptsdi==0) {
        frame=0x540;  /* EOT */
        fvideo=0x40;
      }
    }
    else {
      /* listener */
      disp_char(n);
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
          if (fvideo&0x80) clrdsp();
          break;
        case 20: /* DCL */
          clrdsp();
          break;
      }
      break;
    case 1:     /* LAD */
      n &= 31;
      if (n==31) {
        /* UNL, if not talker then go to idle state */
        if ((fvideo&0x40)==0) fvideo=0;
      }
      else {
        /* else, if MLA go to listen state */
        if (n==(adr&31)) fvideo=0x80;
      }
      break;
    case 2:    /* TAD */
      n &= 31;
      if (n==(adr&31)) {
        /* if MTA go to talker state */
        fvideo=0x40;
      }
      else {
        /* else if addressed talker, go to idle state */
        if ((fvideo&0x40)!=0) fvideo=0;
      }
      break;
    case 4:
      n &= 31;
      switch (n) {
        case 16:  /* IFC */
          fvideo=0;
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
    if (fvideo&0x40) {
      /* if addressed talker */
      if (n==66) {   /* NRD */
        ptsdi=0;     /* abort transfer */
      }
      else if (n==96) { /* SDA */
        /* no response */
      }
      else if (n==97) { /* SST */
        frame=status;
        fvideo=0xc0;  /* active talker */
      }
      else if (n==98) { /* SDI */
        frame=did[0];
        ptsdi=1;
        fvideo=0xc0;
      }
      else if (n==99) { /* SAI */
        frame=AID;
        fvideo=0xc0;
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
/* aff_frame(frame)                           */
/*                                            */
/* display a frame mnemonic (in scope mode)   */
/* ****************************************** */
static void aff_frame(int frame)
{
  static int cpt=0;
  char s[40];
  int n;
  static char *mnemo[] =
    { "DAB", "DSR", "END", "ESR", "CMD", "RDY", "IDY", "ISR"};
  static char *scmd0[] =
    { "NUL", "GTL", "???", "???", "SDC", "PPD", "???", "???",
      "GET", "???", "???", "???", "???", "???", "???", "ELN",
      "NOP", "LLO", "???", "???", "DCL", "PPU", "???", "???",
      "EAR", "???", "???", "???", "???", "???", "???", "???" };
  static char *scmd9[] =
    { "IFC", "???", "REN", "NRE", "???", "???", "???", "???",
      "???", "???", "AAU", "LPD", "???", "???", "???", "???" };

  sprintf(s,"%s %02x  ",mnemo[frame>>8],frame&255);

  if ((frame&0x700)==0x400) {
    /* CMD */
    n=frame&255;
    switch (n/32) {
      case 0:
        sprintf(s,"%s     ",scmd0[n&31]); break;
      case 1:
        sprintf(s,"%s %02x  ","LAD",n&31); break;
      case 2:
        sprintf(s,"%s %02x  ","TAD",n&31); break;
      case 3:
        sprintf(s,"%s %02x  ","SAD",n&31); break;
      case 4:
        if ((n&31)<16)
          sprintf(s,"%s %02x  ","PPE",n&31);
        else
          sprintf(s,"%s     ",scmd9[n&15]);
        break;
      case 5:
        sprintf(s,"%s %02x  ","DDL",n&31); break;
      case 6:
        sprintf(s,"%s %02x  ","DDT",n&31); break;
    }
    if (s[0]=='?') sprintf(s,"%s %02x  ","CMD",n);
  }

  if ((frame&0x700)==0x500) {
    /* RDY */
    n=frame&255;
    if (n<128) {
      switch (n) {
        case  0: sprintf(s,"%s     ","RFC"); break;
        case 64: sprintf(s,"%s     ","ETO"); break;
        case 65: sprintf(s,"%s     ","ETE"); break;
        case 66: sprintf(s,"%s     ","NRD"); break;
        case 96: sprintf(s,"%s     ","SDA"); break;
        case 97: sprintf(s,"%s     ","SST"); break;
        case 98: sprintf(s,"%s     ","SDI"); break;
        case 99: sprintf(s,"%s     ","SAI"); break;
        case 100:sprintf(s,"%s     ","TCT"); break;
      }
    }
    else {
      switch (n/32) {
        case 4:  sprintf(s,"%s %02x  ","AAD",n&31); break;
        case 5:  sprintf(s,"%s %02x  ","AEP",n&31); break;
        case 6:  sprintf(s,"%s %02x  ","AES",n&31); break;
        case 7:  sprintf(s,"%s %02x  ","AMP",n&31); break;
      }
    }
  }

  disp_string(s);
  if (fic!=NULL) {
    fprintf(fic,"%s",s);
    cpt++;
    if (cpt==8) { fprintf(fic,"\n"); cpt=0; }
  }

}

/* ****************************************** */
/* aff_doe(frame)                             */
/*                                            */
/* display a data frame (in listen only mode) */
/* ****************************************** */
static void aff_doe(int frame)
{
  int n;

  n=frame & 255;
  switch (n) {
    case 10:
    case 13: break;
    default:
     if ((n<32)||(n>127))  n='.';
  }
  disp_char(n);
  if ((fic!=NULL)&&(n!=13)) fprintf(fic,"%c",n);
}

/* ****************************************** */
/* ildisplay(frame)                           */
/*                                            */
/* manage a HPIL frame                        */
/* returns the returned frame                 */
/* ****************************************** */
int ildisplay(int frame)
{
  switch (fmonit) {
    case 1:  /* scope */
      aff_frame(frame);
      break;
    case 2:  /* listen-only */
      if ((frame&0x400)==0) aff_doe(frame);
      break;
    default:
      if ((frame&0x400)==0) frame=traite_doe(frame);
      else if ((frame&0x700)==0x400) frame=traite_cmd(frame);
      else if ((frame&0x700)==0x500) frame=traite_rdy(frame);
  }

  return(frame);
}

