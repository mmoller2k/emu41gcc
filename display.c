/* ****************************************** */
/* Emu41: HP-41C software emulator for PC     */
/*                                            */
/* display.c   LCD management module          */
/*                                            */
/* Version 2.5, J-F Garnier 1997-2007         */
/* ****************************************** */

#include <stdio.h>
#include <string.h>

#include "display.h"

#define GLOBAL extern
#include "nutcpu.h"


/* display register definition (experimental support for DSIZE=16) */
#define DSIZE 12
unsigned char lcd_a[DSIZE];
unsigned char lcd_b[DSIZE];
unsigned char lcd_c[DSIZE];
int lcd_ann;  /* announciators */



/* ****************************************** */
/* lcd_rot_right(reg)                         */
/*                                            */
/* rotate a LCD register one nibble right     */
/* ****************************************** */
static void lcd_rot_right (unsigned char *reg)
{
  int t;

  t      = reg[0];
  reg[0] = reg[1];
  reg[1] = reg[2];
  reg[2] = reg[3];
  reg[3] = reg[4];
  reg[4] = reg[5];
  reg[5] = reg[6];
  reg[6] = reg[7];
  reg[7] = reg[8];
  reg[8] = reg[9];
  reg[9] = reg[10];
  reg[10]= reg[11];
#if DSIZE==16
  reg[11] = reg[12];
  reg[12] = reg[13];
  reg[13] = reg[14];
  reg[14] = reg[15];
#endif
  reg[DSIZE-1]= t;
}

/* ****************************************** */
/* lcd_rot_left(reg)                          */
/*                                            */
/* rotate a LCD register one nibble left      */
/* ****************************************** */
static void lcd_rot_left (unsigned char *reg)
{
  int t;

  t = reg[DSIZE-1];
#if DSIZE==16
  reg[15] = reg[14];
  reg[14] = reg[13];
  reg[13] = reg[12];
  reg[12] = reg[11];
#endif  
  reg[11] = reg[10];
  reg[10] = reg[9];
  reg[9]  = reg[8];
  reg[8]  = reg[7];
  reg[7]  = reg[6];
  reg[6]  = reg[5];
  reg[5]  = reg[4];
  reg[4]  = reg[3];
  reg[3]  = reg[2];
  reg[2]  = reg[1];
  reg[1]  = reg[0];
  reg[0]  = t;
}


/* ****************************************** */
/* display_rd_n (n)                           */
/*                                            */
/* LCD read opcodes                           */
/* initially based on NSIM05 from Eric Smith  */
/* ****************************************** */
void display_rd_n (int n)
{
  int i, j;

  j=0;
  switch (n) {
    case 0x0:   /* lcd_rd (A,   12, LEFT); */
      for (i = 0; i < 12; i++) {
        regC[j++] = lcd_a[DSIZE-1];
        lcd_rot_left(lcd_a);
      }
      break;
    case 0x1:   /* lcd_rd (B,   12, LEFT); */
      for (i = 0; i < 12; i++) {
        regC[j++] = lcd_b[DSIZE-1];
        lcd_rot_left(lcd_b);
      }
      break;
    case 0x2:   /* lcd_rd (C,   12, LEFT); */
      for (i = 0; i < 12; i++) {
        regC[j++] = lcd_c[DSIZE-1];
        lcd_rot_left(lcd_c);
      }
      break;
    case 0x3:   /* lcd_rd (AB,   6, LEFT); */
      for (i = 0; i < 6; i++) {
        regC[j++] = lcd_a[DSIZE-1];
        lcd_rot_left(lcd_a);
        regC[j++] = lcd_b[DSIZE-1];
        lcd_rot_left(lcd_b);
      }
      break;
    case 0x4:   /* lcd_rd (ABC,  4, LEFT); */
      for (i = 0; i < 4; i++) {
        regC[j++] = lcd_a[DSIZE-1];
        lcd_rot_left(lcd_a);
        regC[j++] = lcd_b[DSIZE-1];
        lcd_rot_left(lcd_b);
        regC[j++] = lcd_c[DSIZE-1];
        lcd_rot_left(lcd_c);
      }
      break;
    case 0x5:   /* lcd_rd_ann (); */
      regC[2] = (lcd_ann >> 8) & 0x0f;
      regC[1] = (lcd_ann >> 4) & 0x0f;
      regC[0] = lcd_ann & 0x0f;
      j=3;
      break;
    case 0x6:  /* lcd_rd (C,    1, LEFT); */
      regC[j++] = lcd_c[DSIZE-1];
      lcd_rot_left(lcd_c);
      break;
    case 0x7:  /* lcd_rd (A,    1, RIGHT); */
      regC[j++] = lcd_a[0];
      lcd_rot_right(lcd_a);
      break;
    case 0x8:  /* lcd_rd (B,    1, RIGHT); */
      regC[j++] = lcd_b[0];
      lcd_rot_right(lcd_b);
      break;
    case 0x9:  /* lcd_rd (C,    1, RIGHT); */
      regC[j++] = lcd_c[0];
      lcd_rot_right(lcd_c);
      break;
    case 0xa:  /* lcd_rd (A,    1, LEFT); */
      regC[j++] = lcd_a[DSIZE-1];
      lcd_rot_left(lcd_a);
      break;
    case 0xb:  /* lcd_rd (B,    1, LEFT); */
      regC[j++] = lcd_b[DSIZE-1];
      lcd_rot_left(lcd_b);
      break;
    case 0xc:  /* lcd_rd (AB,   1, RIGHT); */
      regC[j++] = lcd_a[0];
      lcd_rot_right(lcd_a);
      regC[j++] = lcd_b[0];
      lcd_rot_right(lcd_b);
      break;
    case 0xd:  /* lcd_rd (AB,   1, LEFT);  */
      regC[j++] = lcd_a[DSIZE-1];
      lcd_rot_left(lcd_a);
      regC[j++] = lcd_b[DSIZE-1];
      lcd_rot_left(lcd_b);
      break;
    case 0xe:  /* lcd_rd (ABC,  1, RIGHT); */
      regC[j++] = lcd_a[0];
      lcd_rot_right(lcd_a);
      regC[j++] = lcd_b[0];
      lcd_rot_right(lcd_b);
      regC[j++] = lcd_c[0];
      lcd_rot_right(lcd_c);
      break;
    case 0xf:  /* lcd_rd (ABC,  1, LEFT); */
      regC[j++] = lcd_a[DSIZE-1];
      lcd_rot_left(lcd_a);
      regC[j++] = lcd_b[DSIZE-1];
      lcd_rot_left(lcd_b);
      regC[j++] = lcd_c[DSIZE-1];
      lcd_rot_left(lcd_c);
      break;
  }
  while (j<14) regC[j++] = 0;
}

/* ****************************************** */
/* display_wr_n (n)                           */
/*                                            */
/* LCD write opcodes                          */
/* initially based on NSIM05 from Eric Smith  */
/* (corrected JFG 14/05/99)                   */
/* ****************************************** */
void display_wr_n (int n)
{
  int i, j;

  j = 0;
  switch (n) {
    case 0x0:  /* lcd_wr (A,   12, LEFT); */
      for (i = 0; i < 12; i++) {
        lcd_rot_right(lcd_a);
        lcd_a[DSIZE-1] = regC[j++];
      }
      break;
    case 0x1:  /* lcd_wr (B,   12, LEFT); */
      for (i = 0; i < 12; i++) {
        lcd_rot_right(lcd_b);
        lcd_b[DSIZE-1] = regC[j++];
      }
      break;
    case 0x2:  /* lcd_wr (C,   12, LEFT); */
      for (i = 0; i < 12; i++) {
        lcd_rot_right(lcd_c);
        lcd_c[DSIZE-1] = regC[j++] & 1;
      }
      break;
    case 0x3:  /* lcd_wr (AB,   6, LEFT); */
      for (i = 0; i < 6; i++) {
        lcd_rot_right(lcd_a);
        lcd_a[DSIZE-1] = regC[j++];
        lcd_rot_right(lcd_b);
        lcd_b[DSIZE-1] = regC[j++];
      }
      break;
    case 0x4:  /* lcd_wr (ABC,  4, LEFT); */
      for (i = 0; i < 4; i++) {
        lcd_rot_right(lcd_a);
        lcd_a[DSIZE-1] = regC[j++];
        lcd_rot_right(lcd_b);
        lcd_b[DSIZE-1] = regC[j++];
        lcd_rot_right(lcd_c);
        lcd_c[DSIZE-1] = regC[j++] & 1;
      }
      break;
    case 0x5:  /* lcd_wr (AB,   6, RIGHT); */
      for (i = 0; i < 6; i++) {
        lcd_rot_left(lcd_a);
        lcd_a[0] = regC[j++];
        lcd_rot_left(lcd_b);
        lcd_b[0] = regC[j++];
      }
      break;
    case 0x6:  /* lcd_wr (ABC,  4, RIGHT); */
      for (i = 0; i < 4; i++) {
        lcd_rot_left(lcd_a);
        lcd_a[0] = regC[j++];
        lcd_rot_left(lcd_b);
        lcd_b[0] = regC[j++];
        lcd_rot_left(lcd_c);
        lcd_c[0] = regC[j++] & 1;
      }
      break;
    case 0x7:  /* lcd_wr (A,    1, LEFT); */
      lcd_rot_right(lcd_a);
      lcd_a[DSIZE-1] = regC[j++];
      break;
    case 0x8:  /* lcd_wr (B,    1, LEFT); */
      lcd_rot_right(lcd_b);
      lcd_b[DSIZE-1] = regC[j++];
      break;
    case 0x9:  /* lcd_wr (C,    1, LEFT); */
      lcd_rot_right(lcd_c);
      lcd_c[DSIZE-1] = regC[j++];
      break;
    case 0xa:  /* lcd_wr (A,    1, RIGHT); */
      lcd_rot_left(lcd_a);
      lcd_a[0] = regC[j++];
      break;
    case 0xb:  /* lcd_wr (B,    1, RIGHT); */
      lcd_rot_left(lcd_b);
      lcd_b[0] = regC[j++];
      break;
    case 0xc:  /* lcd_wr (AB,    1, LEFT); corrected JFG 14/05/99*/
      lcd_rot_right(lcd_a);
      lcd_a[DSIZE-1] = regC[j++];
      lcd_rot_right(lcd_b);
      lcd_b[DSIZE-1] = regC[j++];
      break;
    case 0xd:  /* lcd_wr (AB,   1, RIGHT); */
      lcd_rot_left(lcd_a);
      lcd_a[0] = regC[j++];
      lcd_rot_left(lcd_b);
      lcd_b[0] = regC[j++];
      break;
    case 0xe:  /* lcd_wr (ABC,  1, LEFT); */
      lcd_rot_right(lcd_a);
      lcd_a[DSIZE-1] = regC[j++];
      lcd_rot_right(lcd_b);
      lcd_b[DSIZE-1] = regC[j++];
      lcd_rot_right(lcd_c);
      lcd_c[DSIZE-1] = regC[j++] & 1;
      break;
    case 0xf:  /* lcd_wr (ABC,  1, RIGHT); */
      lcd_rot_left(lcd_a);
      lcd_a[0] = regC[j++];
      lcd_rot_left(lcd_b);
      lcd_b[0] = regC[j++];
      lcd_rot_left(lcd_c);
      lcd_c[0] = regC[j++] & 1;
      break;
  }
}

/* ****************************************** */
/* display_wr ()                              */
/*                                            */
/* LCD announciator write opcode              */
/* ****************************************** */
void display_wr (void)
{
  lcd_ann = (regC[2] << 8) | (regC[1] << 4) | regC[0];
}


/* ****************************************** */
/* init_display ()                            */
/*                                            */
/* init LCD registers                         */
/* ****************************************** */
void init_display (void)
{
  int i;

  for (i = 0; i < DSIZE; i++)
    lcd_a[i] = lcd_b[i] = lcd_c[i] = 0;
  lcd_ann = 0;
}


/* map of punctuation signs */
static int punct_map [4] = { ' ', '.', ':', ',' };


/* ****************************************** */
/* ann_to_buf (buf)                           */
/*                                            */
/* build the announciator output buffer       */
/*   from the announciator register           */
/* buf: 36-byte buffer                        */
/* ****************************************** */
char *ann_to_buf (char *buf)
{
static int last_ann=0;
static char lastbuf[40]="";

  if (lcd_ann==last_ann)
    strcpy(buf,lastbuf);
  else {
/*    "012345678901234567890123456789012345"
      "BAT USER GRAD SHIFT 01234 PRGM ALPHA"; */
    strcpy(buf,"                                    ");
    if (lcd_ann&0x800) { buf[0]='B'; buf[1]='A'; buf[2]='T'; }
    if (lcd_ann&0x400) { buf[4]='U'; buf[5]='S'; buf[6]='E'; buf[7]='R'; }
    if (lcd_ann&0x200) buf[9]='G';
    if (lcd_ann&0x100) { buf[10]='R'; buf[11]='A'; buf[12]='D'; }
    if (lcd_ann&0x080) { buf[14]='S'; buf[15]='H'; buf[16]='I'; buf[17]='F'; buf[18]='T'; }
    if (lcd_ann&0x040) buf[20]='0';
    if (lcd_ann&0x020) buf[21]='1';
    if (lcd_ann&0x010) buf[22]='2';
    if (lcd_ann&0x008) buf[23]='3';
    if (lcd_ann&0x004) buf[24]='4';
    if (lcd_ann&0x002) { buf[26]='P'; buf[27]='R'; buf[28]='G'; buf[29]='M'; }
    if (lcd_ann&0x001) { buf[31]='A'; buf[32]='L'; buf[33]='P'; buf[34]='H'; buf[35]='A'; }
    last_ann=lcd_ann;
    strcpy(lastbuf,buf);
  }
  return (buf);
}

/* ****************************************** */
/* alpha1(v)                                  */
/*                                            */
/* translate a HP41 char code to an           */
/*  equivalent ASCII char code                */
/* ****************************************** */
static char alpha41(int v)
{
  char c;

  v &=0x13f;
  if (v<=0x1f)
    c=v+'@';
  else if (v<=0x3f) {
    if (v==0x2c) c='<';  /* backward flying goose */
    else if (v==0x2e) c='>';  /* flying goose */
    else if (v==0x3a) c='*';  /* starburst */
    else c=v;
  }
  else if (v<=0x105)
    c=v-0xa0;
  else if (v<=0x11f)
    if (v==0x106) c='~';       /* top bar */
    else if (v==0x107) c='\''; /* append */
    else if (v==0x10c) c='u';  /* micro */
    else if (v==0x10d) c='#';  /* different sign */
    else if (v==0x10e) c='s';  /* sigma */
    else if (v==0x10f) c='a';  /* angle */
    else c='x'; /* non-displayable, sorry */
  else
    c=v-0x120+'a'-1;
  return(c);
}



/* ****************************************** */
/* display_to_buf (buf)                       */
/*                                            */
/* build the LCD output buffer                */
/*   from the LCD registers                   */
/* buf: 24-byte display buffer                */
/*      (32-byte with option DSIZE=16)        */
/* ****************************************** */
char *display_to_buf (char *buf)
{
  int i;
  int c;
  char *s;
  
  s = buf;
  
  for (i = DSIZE-1; i >= 0; i--) {
    c= (lcd_c [i] << 8) + ((lcd_b [i] & 3) <<4 )  + lcd_a [i];  /* hp41 char code */
    *s++ = alpha41(c);  /* put equivalent ASCII representation into buffer */
    *s++ = punct_map [lcd_b [i] >> 2];  /* add punctuation sign */
  }
  *s++ = 0;
  return (buf);
}

