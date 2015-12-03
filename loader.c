/* ****************************************** */
/* Emu41: HP-41C software emulator for PC     */
/*                                            */
/* loader.c   ROM/RAM and HPIL device loader  */
/*                                            */
/* Version 2.5, J-F Garnier 1997-2009         */
/* ****************************************** */

/* Conditional compilation:
   VERS_ASM: use nutcpu2.asm
   VERS_LAP: disable FDISC1, LPRTER1, SERIAL2 (for Portable Plus and LX)
   XIL: support of external HP-IL with HP-IL/PC board
   HPPLUS: specific for the HP Portable 110 plus
*/

#include <stdio.h>
#include <dos.h>
#include <conio.h>
#include <mem.h>
#include <alloc.h>
#include <stdlib.h>
#include <string.h>

#include "version.h"


char f_xil;   /* flag XIL installed */

#define GLOBAL extern
#include "nutcpu.h"

/* assembly language routines to speed-up module loading on slow machines (Emu41lx/Emu41p) */
extern  loadunpack(int far *ptri,char *ptrc);
extern  load1kpack(int far *ptri,char *ptrc);

/* conditional for Borland Turbo C 1.0 (used for Emu41p/Emu41lx) */
#if __TURBOC__ < 0x200
void _fmemcpy(int far *dest, int far *src, int n)
{
  n /= 2;
  while (n--) *dest++ = *src++;
}
#endif



/* ****************************************** */
/* initcpu()                                  */
/*                                            */
/* init CPU registers and internal states     */
/* ****************************************** */
void initcpu(void)
{
  int i;

  for (i=0;i<14;i++) {
    regA[i]=0;
    regB[i]=0;
    regC[i]=0;
    regM[i]=0;
    regN[i]=0;
  }
  for (i=0;i<4;i++) {
    retstk[i]=0;
  }
  regPC=0;
  regST=0x0800;   /* stack lift enabled */
  regPQ[0]=0;
  regPQ[1]=0;
  regG=0;
  Carry=1;      /* coldstart ! */
  regData=0;
  regPer=0;

  regPT=0;
  flagKB=0;
  smartp=0;
  selper=0;

  dspon=0;
/*  fslow=0;*/
  facces_dsp=0;
  mode_printer=-1;  /* OFF */
  flagPrter=0;

  cptinstr=0;
  lgkeybuf=0;

  for (i=0;i<16;i++) {
    tabpage[i]=0;
    typmod[i]=0;
    tabbank[i][0]=0;
    tabbank[i][1]=0;
    tabbank[i][2]=0;
    tabbank[i][3]=0;
    fhram[i]=0;
  }

  nbdev=0; /* number of il devices */


}


/* ****************************************** */
/* loadRAM()                                  */
/*                                            */
/* load main and X-registers                  */
/* Note: Hepax, MLDL, RAMBOX, RSU RAM are     */
/*   loaded as normal ROM modules             */
/* Note: 7-byte registers are saved as        */
/*   8-byte strings in memory files           */
/* ****************************************** */
void loadRAM(void)
{
  FILE *fic;
  unsigned char *ptr;
  int  i;

  memset(espaceRAM,0,8192);
  fic=fopen("MEM41.DAT","rb");
  if (fic!=NULL) {
    fread(&espaceRAM[0],8,16,fic);       /* 16 reg. status */
    fread(&espaceRAM[192*8],8,320,fic);  /* 320 reg. main */
    fclose(fic);
  }
  fic=fopen("XMEM41.DAT","rb");
  if (fic!=NULL) {
    fread(&espaceRAM[64*8],8,128,fic);   /* 128 reg. XF */
    fread(&espaceRAM[513*8],8,239,fic);  /* 239 reg. XM1 */
    fread(&espaceRAM[769*8],8,239,fic);  /* 239 reg. XM2 */
    fclose(fic);
  }
  /* mark non-existing registers  */
  for (i=16;i<=63;i++) {
    ptr= &espaceRAM[i*8];
    *ptr=1;
  }
  ptr= &espaceRAM[512*8];
  *ptr=1;
  for (i=752;i<=768;i++) {
    ptr= &espaceRAM[i*8];
    *ptr=1;
  }
  for (i=1008;i<=1023;i++) {
    ptr= &espaceRAM[i*8];
    *ptr=1;
  }


}

/* ****************************************** */
/* saveRAM()                                  */
/*                                            */
/* save main and X-registers                  */
/* if applicable, save Hepax, MLDL, RAM-BOX,  */
/*   and RSU RAM                              */
/* ****************************************** */
void saveRAM(void)
{
  FILE *fic;
  int i, j, k, n;
  unsigned char *ptrc;
  unsigned short *ptri;
  char buf[1280];

  /* test if prgm pointer in ROM */
  if (regST&0x400) {
    /* yes, set prgm pointer to .END. */
    i=espaceRAM[13*8+1];
    j=espaceRAM[13*8+2]&15;
    espaceRAM[12*8+1]=i;
    espaceRAM[12*8+2]=j+3*16;
    /* and set line pointer to 0 */
    espaceRAM[15*8+1]=0;
    espaceRAM[15*8+2]&=0xf0;
  }

  fic=fopen("MEM41.DAT","wb");
  if (fic!=NULL) {
    fwrite(&espaceRAM[0],8,16,fic);       /* 16 reg. status */
    fwrite(&espaceRAM[192*8],8,320,fic);  /* 320 reg. main */
    fclose(fic);
  }
  else
    printf("Emu41: error writing MEM41.DAT\n");

  fic=fopen("XMEM41.DAT","wb");
  if (fic!=NULL) {
    fwrite(&espaceRAM[64*8],8,128,fic);   /* 128 reg. XF */
    fwrite(&espaceRAM[513*8],8,239,fic);  /* 239 reg. XM1 */
    fwrite(&espaceRAM[769*8],8,239,fic);  /* 239 reg. XM2 */
    fclose(fic);
  }
  else
    printf("Emu41: error writing XMEM41.DAT\n");

  for (i=8;i<16;i+=2) {
    fic=NULL;
    if (typmod[i]==5) {
      /* HEPAX RAM */
      fic=fopen("HEPAXRAM.DAT","wb");
    }
    if (typmod[i]==7) {
      /* MLDL RAM */
      fic=fopen("MLDLRAM.DAT","wb");
    }
    if (fic!=NULL) {
      //ptri=MK_FP(tabpage[i],0);
      ptri=tabpage[i];
      for (j=0;j<8;j++) {
        ptrc = buf;
        for (k=0;k<256;k++) {
          *ptrc++ =  ptri[0]    &0xff;
          *ptrc++ =((ptri[1]<<2)&0xfc) + ((ptri[0]>>8)&0x03);
          *ptrc++ =((ptri[2]<<4)&0xf0) + ((ptri[1]>>6)&0x0f);
          *ptrc++ =((ptri[3]<<6)&0xc0) + ((ptri[2]>>4)&0x3f);
          *ptrc++ =                      ((ptri[3]>>2)&0xff);
          ptri += 4;
        }
        n=fwrite(buf,1,1280,fic);
        if (n!=(1280)) {
          printf("Err: error writing RAM file\n");
          break;
        }
      }
      fclose(fic);
    }
  }

  if (typmod[8]==9) {
    /* RAMBOX II 64k */
    fic=fopen("RAMBOXII.DAT","wb");
    if (fic!=NULL) {
      for (j=0;j<64;j++) {
        //ptri=MK_FP(tabbank[8][0]+j*128,0);
	ptri = tabbank[8][0]+j*512;
        ptrc = buf;
        for (k=0;k<256;k++) {
          *ptrc++ =  ptri[0]    &0xff;
          *ptrc++ =((ptri[1]<<2)&0xfc) + ((ptri[0]>>8)&0x03);
          *ptrc++ =((ptri[2]<<4)&0xf0) + ((ptri[1]>>6)&0x0f);
          *ptrc++ =((ptri[3]<<6)&0xc0) + ((ptri[2]>>4)&0x3f);
          *ptrc++ =                      ((ptri[3]>>2)&0xff);
          ptri += 4;
        }
        n=fwrite(buf,1,1280,fic);
        if (n!=(1280)) {
          printf("Err: error writing RAM file\n");
          break;
        }
      }
      fclose(fic);
    }
  }

  if (typmod[8]==11) {
    /* Eramco ES RSU "128k" */
    fic=fopen("ES_RSU.DAT","wb");
    if (fic!=NULL) {
      for (j=0;j<96;j++) {
        //ptri=MK_FP(tabbank[8][0]+j*128,0);
	ptri=tabbank[8][0]+j*512;
        ptrc = buf;
        for (k=0;k<256;k++) {
          *ptrc++ =  ptri[0]    &0xff;
          *ptrc++ =((ptri[1]<<2)&0xfc) + ((ptri[0]>>8)&0x03);
          *ptrc++ =((ptri[2]<<4)&0xf0) + ((ptri[1]>>6)&0x0f);
          *ptrc++ =((ptri[3]<<6)&0xc0) + ((ptri[2]>>4)&0x3f);
          *ptrc++ =                      ((ptri[3]>>2)&0xff);
          ptri += 4;
        }
        n=fwrite(buf,1,1280,fic);
        if (n!=(1280)) {
          printf("Err: error writing RAM file\n");
          break;
        }
      }
      fclose(fic);
    }
  }

}






/* module types:
   1=ROM, 2=CXTIME, 3=12k w. BS (Advantage), 4=HEPAX ROM, 5=HEPAX RAM, 6=IR printer, 
   7=MLDL RAM, 8=W&W ROMBOX, 9=W&W RAMBOX, 10=ES ROM Storage Unit, 11=ES RAM Storage Unit */

/* ****************************************** */
/* loadmodule(nupage, taille, nomfic)         */
/*                                            */
/* load ROM or RAM modules (see types above)  */
/* nupage: target page                        */
/*        (first page for multi-page modules) */
/* taille: size in kwords                     */
/* nomfic: file name                          */
/* ****************************************** */
void loadmodule(int nupage, int taille,char *nomfic)
{
  FILE *fic;
  unsigned short *ptri;
  unsigned char *ptrc;
  int i, j, n, fr;
  char buf[1280];  /* packed 1kword buffer */
  long lg;


  printf("Loading %s\n",nomfic);
  fr=0;
  ptrc=strchr(nomfic,'.');
  if (ptrc!=0) {
/*    if (stricmp(ptrc,"rom")==0) fr=1; */
  }
  if ((nupage<0)||(nupage>15)) return;
  
  /* allocate memory */
  //ptri=(unsigned int far *) farmalloc(16+taille*2*1024L);
  ptri=(unsigned short *) malloc(taille*2*1024L);
  if (ptri==NULL) {
    printf("Err: not enough memory for %s\n",nomfic);
    return;
  }
  /* security */
  if (FP_OFF(ptri)>15) {
    printf("Emu41: invalid memory pointer\n");
    exit(0);
  }
  /* build page pointer into page table, and init module type to ROM */
  //tabpage[nupage]=FP_SEG(ptri)+1;
  tabpage[nupage] = ptri;
  typmod[nupage]=1;   /* ROM */

  /* init page(s) to zero */
  memset(buf,0,1280);
  for (i=0;i<taille;i++) {
    //ptri = MK_FP(tabpage[nupage]+i*128,0);
    ptri = tabpage[nupage]+i*512;
    _fmemcpy(ptri,buf,1024); /* write 512 words */
    ptri += 512; /* bug fix 24/12/03 */
    _fmemcpy(ptri,buf,1024); /* write 512 words */
  }

  /* read file into target pages */
  fic=fopen(nomfic,"rb");
  if (fic==NULL) {
    printf("Err: file %s not found\n",nomfic);
  }
  else {
    /* recognize file pack or unpack format */
    fseek(fic,0,SEEK_END);
    lg=ftell(fic);
    fseek(fic,0,SEEK_SET);
    if ( ((lg/taille)==2048) || ((lg/taille)==4096) ) fr=1;  /* unpack */
    for (i=0;i<taille;i++) {
      //ptri = MK_FP(tabpage[nupage]+i*128,0);
      ptri=tabpage[nupage]+i*512;
      if (fr) {
        n=fread(buf,1,1024,fic);
#ifdef VERS_ASM
       loadunpack(ptri,buf);
       ptri += 512;
#else
        ptrc = buf;
        for (j=0;j<512;j++) {
          *ptri++ = ((ptrc[0]&3)<<8)+ptrc[1];
          ptrc += 2;
       }
#endif
        n=fread(buf,1,1024,fic);
        if (n!=1024) {
          printf("Err: error reading file %s\n",nomfic);
          break; /* leave loop */
        }
#ifdef VERS_ASM
        loadunpack(ptri,buf);
#else
        ptrc = buf;
        for (j=0;j<512;j++) {
          *ptri++ = ((ptrc[0]&3)<<8)+ptrc[1];
          ptrc += 2;
        }
#endif
      }
      else {
        n=fread(buf,1,1280,fic);
        if (n!=1280) {
          printf("Err: error reading file %s\n",nomfic);
          break; /* leave loop */
        }
#ifdef VERS_ASM
        load1kpack(ptri,buf);
#else
        ptrc = buf;
        for (j=0;j<256;j++) {
          *ptri++ = ((ptrc[0]&0xff)   ) + ((ptrc[1]&0x03)<<8);
          *ptri++ = ((ptrc[1]&0xfc)>>2) + ((ptrc[2]&0x0f)<<6);
          *ptri++ = ((ptrc[2]&0xf0)>>4) + ((ptrc[3]&0x3f)<<4);
          *ptri++ = ((ptrc[3]&0xc0)>>6) + ((ptrc[4]&0xff)<<2);
          ptrc += 5;
        }
#endif
      }
    }
    fclose(fic);
  }

  /* init page and bank pointers, and module type */
  if ((nupage==0)&&(taille==12)) {
    /* internal ROM */
    tabpage[1]=tabpage[0]+512; /* 512 == 8192/16 */
    tabpage[2]=tabpage[1]+512;
    typmod[1]=1;
    typmod[2]=1;
  }
  else if ((nupage==5)&&(taille==8)) {
    /* CXTIME */
    typmod[3]=2;
    typmod[5]=2;
    tabbank[5][0]=tabpage[5];
    tabbank[5][1]=tabpage[5]+512;
  }
  else if ((nupage>=8)&&(nupage<=14)&&(taille==12)) {
    /* 12k bank switching module, e.g. Advantage */
    typmod[nupage]=3;
    typmod[nupage+1]=3;
    tabbank[nupage+1][0]=tabpage[nupage]+512;
    tabbank[nupage+1][1]=tabpage[nupage]+1024;
    tabpage[nupage+1]=tabbank[nupage+1][0];  /* select bank 0 */
  }
  else if ((nupage>=8)&&(taille==16)) {
    /* HEPAX */
    typmod[nupage]=4;
    tabbank[nupage][0]=tabpage[nupage];
    tabbank[nupage][1]=tabpage[nupage]+512;
    tabbank[nupage][2]=tabpage[nupage]+1024;
    tabbank[nupage][3]=tabpage[nupage]+1536;

    if ((nupage&1)==0) {
      /* allocate Hepax RAM */
      //ptri=(unsigned int far *) farmalloc(16+2*8*1024);  /* 8Kwords */
      ptri=(unsigned short *) malloc(2*8*1024);  /* 8Kwords */
      if (ptri==NULL) {
        printf("Err: not enough memory for HEPAX RAM\n");
        return;
      }
      if (FP_OFF(ptri)>15) {
        printf("Emu41: invalid memory pointer\n");
        exit(0);
      }
      //ptri = MK_FP(FP_SEG(ptri)+1,0);
      //tabpage[nupage+1]=FP_SEG(ptri)+512;
      tabpage[nupage+1]=ptri+512;
      typmod[nupage+1]=5;
      fic=fopen("HEPAXRAM.DAT","rb");
      if (fic!=NULL) {
        for (i=0;i<8;i++) {
          n=fread(buf,1,1280,fic);
          if (n!=1280) {
            printf("Err: error reading file HEPAXRAM.DAT\n");
            break;
          }
          ptrc = buf;
          for (j=0;j<256;j++) {
            *ptri++ = ((ptrc[0]&0xff)   ) + ((ptrc[1]&0x03)<<8);
            *ptri++ = ((ptrc[1]&0xfc)>>2) + ((ptrc[2]&0x0f)<<6);
            *ptri++ = ((ptrc[2]&0xf0)>>4) + ((ptrc[3]&0x3f)<<4);
            *ptri++ = ((ptrc[3]&0xc0)>>6) + ((ptrc[4]&0xff)<<2);
            ptrc += 5;
          }
        }
        fclose(fic);
      }
      else {
        memset(buf,0,1280);
        for (i=0;i<8;i++) {
          _fmemcpy(ptri,buf,1024); /* write 512 words */
          ptri += 512; /* bug fix 24/12/03 */
          _fmemcpy(ptri,buf,1024); /* write 512 words */
          ptri += 512; /* bug fix 24/12/03 */
        }
      }
    }
  }
  else if ((nupage==6)&&(taille==8)) {
    /* IR Printer */
    typmod[nupage]=6;
    tabbank[nupage][0]=tabpage[nupage];
    tabbank[nupage][1]=tabpage[nupage]+512;
  }
  else if ((nupage>0)&&(nupage<=14)&&(taille==8)) {
    /* module 8k */
    tabpage[nupage+1]=tabpage[nupage]+512;
    typmod[nupage+1]=1;
  }
  else if ((nupage==8)&&(taille==64)) {
    /* RAMBOX II 64k */
    tabpage[9]=tabpage[8]+512;
    tabpage[10]=tabpage[9]+512;
    tabpage[11]=tabpage[10]+512;
    tabpage[12]=tabpage[11]+512;
    tabpage[13]=tabpage[12]+512;
    tabpage[14]=tabpage[13]+512;
    tabpage[15]=tabpage[14]+512;

    tabbank[8][0]=tabpage[8];
    tabbank[9][0]=tabbank[8][0]+512;
    tabbank[10][0]=tabbank[9][0]+512;
    tabbank[11][0]=tabbank[10][0]+512;
    tabbank[12][0]=tabbank[11][0]+512;
    tabbank[13][0]=tabbank[12][0]+512;
    tabbank[14][0]=tabbank[13][0]+512;
    tabbank[15][0]=tabbank[14][0]+512;
    tabbank[8][1]=tabbank[15][0]+512;
    tabbank[9][1]=tabbank[8][1]+512;
    tabbank[10][1]=tabbank[9][1]+512;
    tabbank[11][1]=tabbank[10][1]+512;
    tabbank[12][1]=tabbank[11][1]+512;
    tabbank[13][1]=tabbank[12][1]+512;
    tabbank[14][1]=tabbank[13][1]+512;
    tabbank[15][1]=tabbank[14][1]+512;

    typmod[8]=8;
    if (stricmp(nomfic,"RAMBOXII.DAT")==0) typmod[8]=9;
    typmod[9]=typmod[8];
    typmod[10]=typmod[8];
    typmod[11]=typmod[8];
    typmod[12]=typmod[8];
    typmod[13]=typmod[8];
    typmod[14]=typmod[8];
    typmod[15]=typmod[8];

  }

  else if ((nupage==8)&&(taille==96)) {
    /* Eramco ES RSU "128k" */
    tabpage[9]=tabpage[8]+512;
    tabpage[10]=tabpage[9]+512;
    tabpage[11]=tabpage[10]+512;
    tabpage[12]=tabpage[11]+512;
    tabpage[13]=tabpage[12]+512;

    tabbank[8][0]=tabpage[8];
    tabbank[9][0]=tabbank[8][0]+512;
    tabbank[10][0]=tabbank[9][0]+512;
    tabbank[11][0]=tabbank[10][0]+512;
    tabbank[12][0]=tabbank[11][0]+512;
    tabbank[13][0]=tabbank[12][0]+512;

    tabbank[8][1]=tabbank[13][0]+512;
    tabbank[9][1]=tabbank[8][1]+512;
    tabbank[10][1]=tabbank[9][1]+512;
    tabbank[11][1]=tabbank[10][1]+512;
    tabbank[12][1]=tabbank[11][1]+512;
    tabbank[13][1]=tabbank[12][1]+512;

    tabbank[8][2]=tabbank[13][1]+512;
    tabbank[9][2]=tabbank[8][2]+512;
    tabbank[10][2]=tabbank[9][2]+512;
    tabbank[11][2]=tabbank[10][2]+512;
    tabbank[12][2]=tabbank[11][2]+512;
    tabbank[13][2]=tabbank[12][2]+512;

    tabbank[8][3]=tabbank[13][2]+512;
    tabbank[9][3]=tabbank[8][3]+512;
    tabbank[10][3]=tabbank[9][3]+512;
    tabbank[11][3]=tabbank[10][3]+512;
    tabbank[12][3]=tabbank[11][3]+512;
    tabbank[13][3]=tabbank[12][3]+512;

    typmod[8]=10;
    if (stricmp(nomfic,"ES_RSU.DAT")==0) typmod[8]=11;
    typmod[9]=typmod[8];
    typmod[10]=typmod[8];
    typmod[11]=typmod[8];
    typmod[12]=typmod[8];
    typmod[13]=typmod[8];

  }

  if (stricmp(nomfic,"MLDLRAM.DAT")==0) {
    typmod[nupage]=7;
    if (taille==8) typmod[nupage+1]=7;
  }

  if ((nupage==6)&&(taille==4)) {
    /* test if HP82143A printer installed */
    fetch1(0);
    if ((fetch1(0x6ffc)==0x31)&&(fetch1(0x6ffb)==0x05))
      /* version 1E */
      mode_printer=0;
  }

  return;
}



/* ****************************************** */
/* loaddevice(nomdev, option)                 */
/*                                            */
/* load HPIL devices                          */
/* nomdev: file name                          */
/* option: specific device options            */
/* ****************************************** */
void loaddevice(char *nomdev, char *option)
{
  int tdev;
  int d, n;

  f_xil=0;   /* XIL not installed by default */

  if (stricmp(nomdev,"DISPLAY")==0) tdev=1;
  else if (stricmp(nomdev,"HDRIVE1")==0) tdev=2;
  else if (stricmp(nomdev,"FDRIVE1")==0) tdev=3;
  else if (stricmp(nomdev,"DOSLINK")==0) tdev=4;
  else if (stricmp(nomdev,"XIL")==0) {
#ifndef HPPLUS
    if ((option[0]=='c')||(option[0]=='C'))
     tdev=10;
    else
#endif
     tdev=5;
  }
  else if (stricmp(nomdev,"LPRTER1")==0) tdev=6;
  else if (stricmp(nomdev,"HDRIVE2")==0) tdev=7;
  else if (stricmp(nomdev,"SERIAL1")==0) tdev=8;
  else if (stricmp(nomdev,"SERIAL2")==0) tdev=9;
  /* XIL COMx is tdev=10 */
  else tdev=0;

  /* add device to table */
  if (tdev && (nbdev<32)) {
    tabdev[nbdev]=tdev;
    nbdev++;
  }
  
  /* specific device init */
  switch (tdev) {
    case 1:
      init_ildisplay();
      break;
    case 2:
      init_ilhdisc(option);
      break;
    case 3:
#ifndef VERS_LAP
      if ((option[0]=='b')||(option[0]=='B')) d=1; else d=0;
      init_ilfdisc(d);
#endif
      break;
    case 5:
#ifdef XIL
      d=0;
      if (option[0]!=0) sscanf(option,"%x",&d);
      if (d==0) d=0x1700;
  #ifdef HPPLUS
      d=0x21;
  #endif
      init_ilext(d,1);
      f_xil=1;   /* XIL installed */
#endif
      break;
    case 6:
#ifndef VERS_LAP
      if ((option[0]=='d')||(option[0]=='D')) d=1; else d=0;
      init_ilcentro(d);
#endif
      break;
    case 7:
#ifndef VERS_LAP
      init_ilhdisc2(option);
#endif
      break;
    case 8:
      d=0;
#ifndef HPPLUS
      n=0xe3;
#else
      n=0;
#endif
      if (option[0]!=0) {
        if (option[1]==0) {
          sscanf(option,"%d",&d);
          d--;
        }
        else if (option[1]==',') {
          sscanf(option,"%d,%x",&d,&n);
          d--;
        }
        else {
          sscanf(option,"%x",&n);
        }
      }
      init_ilserial1(d,n);
      break;
    case 9:
#ifndef VERS_LAP
      d=1;
      n=0xe3;
      if (option[0]!=0) {
        if (option[1]==0) {
          sscanf(option,"%d",&d);
          d--;
        }
        else if (option[1]==',') {
          sscanf(option,"%d,%x",&d,&n);
          d--;
        }
        else {
          sscanf(option,"%x",&n);
        }
      }
      init_ilserial2(d,n);
#endif
      break;
    case 10:  /* XIL2 */
#ifndef HPPLUS
      if ((option[1]!=0)&&(option[2]!=0)&&(option[3]!=0)) {
        sscanf(&option[3],"%d",&d);
        init_ilext2(d,1);
      }
#endif
      break;
  }

}




/* ****************************************** */
/* init_emulate()                             */
/*                                            */
/* init emulator                              */
/* read file EMU41.INI                        */
/* load modules, HPIL devices and RAM         */
/* ****************************************** */
void init_emulate(void)
{
  int n;
  FILE *fic;
  char ligne[82];
  int nupage, taille;
  char nomfic[80];
  char nomdev[40];
  char option[80];
  char f;

  initcpu();
  init_timer();
  init_hpil();

  /* read file .ini, and load modules/devices */
  f=1;
  fic=fopen("emu41.ini","rt");
  if (fic==NULL) {
    printf("Emu41: file emu41.ini not found !\n");
  }
  else {
    while (fgets(ligne,80,fic)) {
      if ( (ligne[0]!=';') && (ligne[0]!=' ') && (ligne[0]!='\n') ) {
        if (ligne[0]=='[') {
          if (strnicmp(&ligne[1],"MOD",3)==0)
            f=1;
          else if (strnicmp(&ligne[1],"DEV",3)==0)
            f=2;
        }
        else {
          switch (f) {
          case 1:  /* modules */
            n=sscanf(ligne,"%d %d %s",&nupage,&taille,nomfic);
            if (n==3) {
              loadmodule(nupage, taille,nomfic);
            }
            break;
          case 2:  /* devices */
            option[0]=0;
            n=sscanf(ligne,"%s %s",nomdev,option);
            if (n>=1) {
              loaddevice(nomdev, option);
            }
            break;
          }
        }
      }
    }

    fclose(fic);
  }

  if (tabpage[0]==0) {
    printf("Emu41: Error, no system ROM !\n");
    exit(0);
  }

  loadRAM();

  push_key(0x18);     /* press ON to startup */



}



