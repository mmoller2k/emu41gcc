/* ****************************************** */
/* Emu41: HP-41C software emulator for PC     */
/*                                            */
/* desas41.c   HP-41 micro code disassembler  */
/*                                            */
/* Version 2.5, J-F Garnier 1997-2007         */
/* ****************************************** */

/* Conditional compilation:
  HPPLUS: specific for the HP Portable 110 plus
*/

#include <string.h>
#include <stdio.h>

/* external function in trans.c */
extern int ch_label(long adr,char *s);


static int smartp=0;   /* smart peripheral flag */
static int fmsg=0;     /* message data flag */

/* ****************************************** */
/* desas0(codes, ligne)                       */
/*                                            */
/* disassemble opcode group 0 (control)       */
/* codes: opcode array                        */
/* ligne: output buffer                       */
/* returns the length of disassembled opcode  */
/* ****************************************** */
int desas0(int *codes, char *ligne)
{
  int c;
  int opc, par;
  int lg;

  char *mnemo0[] = { "NOP",    "WMLDL",  "(brk)",  "FPU",
                     "ENROM1", "ENROM3", "ENROM2", "ENROM4" } ;

  char *mnemo6[] = { "???",    "G=C",    "C=G",    "CGEX",
                     "???",    "M=C",    "C=M",    "CMEX",
                     "???",    "F=SB",   "SB=F",   "FEXSB",
                     "???",    "ST=C",   "C=ST",   "CSTEX" };

  char *mnemo8[] = { "SPOPND", "POWOFF", "SELP",   "SELQ",
                     "?P=Q",   "LLD",    "CLRABC", "GOTOC",
                     "C=KEYS", "SETHEX", "SETDEC", "DISOFF",
                     "DISTOG", "RTNC",   "RTNNC",  "RTN" };


  char *mnemo12[]= { "ROMBLK", "N=C",    "C=N",    "CNEX",
                     "LDI",    "STK=C",  "C=STK",  "WPTOG",
                     "GOKEYS", "DADD=C", "CLREGS", "DATA=C",
                     "CXISA",  "C=C!A",  "C=C&A",  "PFAD=C" };
/* note: ROMBLK (Hepax) is DISBLK (=disp blink),
   on some other Nut-based calcs.
   CLREGS is not implemented */

/* array for PT and ST arguments */
  int coded[16] = {3,4,5,10,8,6,11,15,2,9,7,13,1,12,0,14 };

  char registres[]="TZYXLMNOPQ.abcde";  /* user register mnemo */

  c=codes[0]>>2;
  opc=c&15;
  par=c>>4;
  lg=1;

  switch (opc) {
    case 0:
      if (par==3) {
        sprintf(ligne,"FPU    %d",codes[1]);   /* FPU extension (Emu41) */
        lg=2;
      }
      else if (par<8)
        strcpy(ligne,mnemo0[par]);
      else
        sprintf(ligne,"HPIL=C %d",par-8);
      break;
    case 1:
      if (par==15)
        strcpy(ligne,"CLRST");
      else
        sprintf(ligne,"ST=0   %d",coded[par]);
      break;
    case 2:
      if (par==15)
        strcpy(ligne,"RSTKB");
      else
        sprintf(ligne,"ST=1   %d",coded[par]);
      break;
    case 3:
      if (par==15)
        strcpy(ligne,"CHKKB");
      else
        sprintf(ligne,"?ST=1  %d",coded[par]);
      break;
    case 4:
      sprintf(ligne,  "LC     %X",par);
      break;
    case 5:
      if (par==15)
        strcpy(ligne,"DECPT");
      else
        sprintf(ligne,"?PT=   %d",coded[par]);
      break;
    case 6:
      strcpy(ligne,mnemo6[par]);
      break;
    case 7:
      if (par==15)
        strcpy(ligne,"INCPT");
      else
        sprintf(ligne,"PT=    %d",coded[par]);
      break;
    case 8:
      strcpy(ligne,mnemo8[par]);
      if (par==1) lg=2;  /* cas powoff */
      break;
    case 9:
      sprintf(ligne,"SELPF  %d",par);
      smartp=1;
      break;
    case 10:
      sprintf(ligne,"REGN=C %d  (%c)",par,registres[par]);
      break;
    case 11:
      sprintf(ligne,"?F%d=1",coded[par]);
      break;
    case 12:
      if (par==4) {
        sprintf(ligne,"LDI    %03X  (%d)",codes[1],codes[1]);
        lg=2;
      }
      else
        strcpy(ligne,mnemo12[par]);
      break;
    case 13:
      strcpy(ligne,"???");
      break;
    case 14:
      if (par==0)
        strcpy(ligne,"C=DATA");
      else
        sprintf(ligne,"C=REGN %d  (%c)",par,registres[par]);
      break;
    case 15:
      if (par!=15) {
        par=coded[par];
        if (par<8)
          sprintf(ligne,  "RCR    %d",par);    /* rotate right (1-7) */
        else
          sprintf(ligne,  "RCL    %d",14-par);  /* rotate left (1-6) */
      }
      else
        sprintf(ligne,  "DISCMP");  /* display temperature compensation */
      break;
  }

  return(lg);
}


/* ****************************************** */
/* desas2(codes, ligne)                       */
/*                                            */
/* disassemble opcode group 2 (arithmetic)    */
/* codes: opcode array                        */
/* ligne: output buffer                       */
/* returns the length of disassembled opcode  */
/* ****************************************** */
int desas2(int *codes, char *ligne)
{
  int  opc, te;

  char *mnemo[] = { "A=0   ", "B=0   ", "C=0   ", "ABEX  ",
                    "B=A   ", "ACEX  ", "C=B   ", "BCEX  ",
                    "A=C   ", "A=A+B ", "A=A+C ", "A=A+1 ",
                    "A=A-B ", "A=A-1 ", "A=A-C ", "C=C+C ",
                    "C=A+C ", "C=C+1 ", "C=A-C ", "C=C-1 ",
                    "C=-C  ", "C=-C-1", "?B#0  ", "?C#0  ",
                    "?A<C  ", "?A<B  ", "?A#0  ", "?A#C  ",
                    "ASR   ", "BSR   ", "CSR   ", "ASL   " };


  char *champ[] = { " PT", " X ", " WPT", " W ", " PQ", " XS", " M ", " S " };


  opc=(codes[0]>>5)&0x1f;
  te=(codes[0]>>2)&0x07;
  strcpy(ligne,mnemo[opc]);
  strcat(ligne,champ[te]);

  return(1);
}




#ifndef HPPLUS
/* not included in Emu41p to save space */

int gsb(int adr, int *codes, char *ligne, int n)
{
  int a;

  a=codes[2];
  if (n>=0) {
    a |= (adr&0xf000) + (n<<10);
    sprintf(ligne,"GSB41C %04X",a);
  }
  else {
    a |= adr&0xfc00;
    sprintf(ligne,"GSBSAM %04X",a);
  }

  return(3);
}

int gto(int adr, int *codes, char *ligne, int n)
{
  int a;

  a=codes[2];
  if (n>=0) {
    a |= (adr&0xf000) + (n<<10);
    sprintf(ligne,"GOL41C %04X",a);
  }
  else {
    a |= adr&0xfc00;
    sprintf(ligne,"GOLSAM %04X",a);
  }

  return(3);
}

#endif

/* ****************************************** */
/* desas1(adr,codes, ligne)                   */
/*                                            */
/* disassemble opcode group 1 (long jump)     */
/* adr: address of the jump opcode            */
/* codes: opcode array                        */
/* ligne: output buffer                       */
/* returns the length of disassembled opcode  */
/* ****************************************** */
int desas1(int adr, int *codes, char *ligne)
{
  int a, res;
  char buf[40];

  a = ((codes[0]>>2)&0xff) + ((codes[1]>>2)<<8);

#ifndef HPPLUS
  switch (a) {
    /* check specific HP41 GOLONG/GOSUB for relative jumps in current page */
    case 0x23d0: res=gto(adr,codes,ligne,0); break;
    case 0x23d2: res=gsb(adr,codes,ligne,0); break;
    case 0x23d9: res=gto(adr,codes,ligne,1); break;
    case 0x23db: res=gsb(adr,codes,ligne,1); break;
    case 0x23e2: res=gto(adr,codes,ligne,2); break;
    case 0x23e4: res=gsb(adr,codes,ligne,2); break;
    case 0x23eb: res=gto(adr,codes,ligne,3); break;
    case 0x23ed: res=gsb(adr,codes,ligne,3); break;
    case 0x0fda: res=gto(adr,codes,ligne,-1); break;
    case 0x0fde: res=gsb(adr,codes,ligne,-1); break;
    default:
#endif
      switch (codes[1]&3) {
        case 0: strcpy(ligne,"GSUBNC "); break;
        case 1: strcpy(ligne,"GSUBC  "); break;
        case 2: strcpy(ligne,"GOLNC  "); break;
        case 3: strcpy(ligne,"GOLC   "); break;
      }
      /* attempt to find a mnemonic label for the target address */
      if (ch_label(a,buf)==0)
        sprintf(buf,"%04X",a);
      strcat(ligne,buf);
      res=2;
      if ((codes[1]&2)==0)
        if ((a==0x7ef)||(a==0x3f83)) fmsg=1;  /* message follows */
#ifndef HPPLUS
  }
#endif
  return(res);
}



/* ****************************************** */
/* desas3(adr,codes, ligne)                   */
/*                                            */
/* disassemble opcode group 3 (short jump)    */
/* adr: address of the jump opcode            */
/* codes: opcode array                        */
/* ligne: output buffer                       */
/* returns the length of disassembled opcode  */
/* ****************************************** */
int desas3(int adr, int *codes, char *ligne)
{
  int a;
  char buf[40];

  if (codes[0]&4)
    strcpy(ligne,"GOC    ");
  else
    strcpy(ligne,"GONC   ");

  a = codes[0]>>3;
  if (a>63) a-=128;
  sprintf(buf,"%04X",adr+a);
  strcat(ligne,buf);

  return(1);
}


/* ****************************************** */
/* desasp(codes, ligne)                       */
/*                                            */
/* disassemble smart peripheral opcodes       */
/* codes: opcode array                        */
/* ligne: output buffer                       */
/* returns the length of disassembled opcode  */
/* ****************************************** */
int desasp(int *codes, char *ligne)
{
  int opc, par;

  if ((codes[0]&2)==0) {
    sprintf(ligne,"CH=   %02X   (%u)",codes[0]>>2,codes[0]>>2);
  }
  else {
    opc=(codes[0]>>2)&15;
    par=(codes[0]>>6)&15;
    switch (opc) {
      case 0: sprintf(ligne,"?PFSET %d",par); break;
      case 1: sprintf(ligne,"PRINTC"); break;
      case 14:sprintf(ligne,"RDPTRN %d",par); break;
      default: sprintf(ligne,"???    %d",par);
    }
  }
  if (codes[0]&1) smartp=0;
  return(1);
}


/* ****************************************** */
/* desas(adr,codes, ligne)                    */
/*                                            */
/* disassemble HP41 opcode                    */
/* adr: address of the opcode                 */
/* codes: opcode array                        */
/* ligne: output buffer                       */
/*   manages smart peripheral opcodes and     */
/*   message strings                          */
/* returns the length of disassembled opcode  */
/* ****************************************** */
int desas(int adr, int *codes, char *ligne)
{
  int  n, c;

  if (smartp) {
    /* smart peripheral code */
    n=desasp(codes,ligne);
  }
  else if (fmsg) {
    /* message string */
    c=codes[0];
    sprintf(ligne,"CON    %03X  (%c)",c,char41(c));
    if (c&0x200) fmsg=0;
    n=1;
  }
  else {
    switch (codes[0]&3) {
      case 0: n=desas0(codes,ligne); break;
      case 1: n=desas1(adr,codes,ligne); break;
      case 2: n=desas2(codes,ligne); break;
      case 3: n=desas3(adr,codes,ligne); break;
    }
  }
  return(n);
}
