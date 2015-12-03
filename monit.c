/* ****************************************** */
/* Emu41: HP-41C software emulator for PC     */
/*                                            */
/* monit.c   Monitor module for Emu41         */
/* (based on my previous Mon41, 1995)         */
/*                                            */
/* Version 2.5, J-F Garnier 1997-2007         */
/* ****************************************** */

/* Conditional compilation:
  HPPLUS: specific for the HP Portable 110 plus
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <conio.h>
#include <time.h>

#include "display.h"

#define GLOBAL extern
#include "nutcpu.h"
#undef GLOBAL

#define GLOBAL extern
#include "timer.h"
#include "hpil.h"
#undef GLOBAL


/* variables for breakpoints */
#define BRKOPC 0x080   /* opcode for breakpoint */
static int lastopcode=0;
unsigned int lastaddr=0xffff;

extern int desas(int adr, int *codes, char *ligne); /* desas41.c */
int ch_label(long adr, char *s);  /* trans.c */
long ch_symb(char *s);



/* ****************************************** */
/* hextoint(c)                                */
/*                                            */
/* convert an HEX character to an int digit   */
/* ****************************************** */
int hextoint(char c)
{
  if (c>'9') c -= 7;
  c -= 48;
  return(c);
}



/* ****************************************** */
/* inttohex(c)                                */
/*                                            */
/* convert an int digit to an HEX character   */
/* ****************************************** */
char inttohex(int c)
{
  c &= 15;
  if (c>9) c += 7;
  return(c+48);
}




/* ****************************************** */
/* litmot(adr)                                */
/*                                            */
/* read a word from address adr               */
/* ****************************************** */
int litmot(unsigned int adr)
{
  fetch1(0);
  return(fetch1(adr));
}


/* ****************************************** */
/* ecritmot(adr, mot)                         */
/*                                            */
/* write the word 'mot' to address adr        */
/* ****************************************** */
void ecritmot(unsigned int adr, int mot)
{
  wmldl1(adr,mot);
}




/* ****************************************** */
/* lit(adr,n,buf)                             */
/*                                            */
/* read n words from address adr to buffer    */
/* ****************************************** */
void lit(unsigned int adr,int n,int *buf)
{
  int i;

  for (i=0;i<n;i++) {
    *buf++ = litmot(adr);
    adr++;
  }

}


/* ****************************************** */
/* char41(v)                                  */
/*                                            */
/* convert a HP41 character code              */
/*   to an equivalent ASCII character         */
/* ****************************************** */
char char41(int v)
{
  char c;

  v &=0x13f;
  if (v<=0x1f)
    c=v+'@';
  else if (v<=0x3f)
    c=v;
  else if (v<0x100)
    c='.';
  else if (v<=0x105)
    c=v-0xa0;
  else if (v<=0x10f)
    c='*';
  else
    c='.';
  return(c);
}

/* ****************************************** */
/* aplha41(dest, hex, n)                      */
/*                                            */
/* convert n HP41 character codes from the    */
/*   array 'hex' to ASCII characters into     */
/*   the array 'dest'                         */
/* ****************************************** */
void alpha41(char *dest,int *hex,int n)
{
  int i, v;

  for (i=0;i<n;i++) {
    v=*hex++;
    *dest++ = char41(v);
  }
  *dest=0;

}


/* ****************************************** */
/* aplha41(dest, hex, n)                      */
/*                                            */
/* convert n hex codes from the               */
/*   array 'hex' to ASCII characters into     */
/*   the array 'dest'                         */
/* ****************************************** */
void alphaASCII(char *dest,int *hex,int n)
{
  int i, v;

  for (i=0;i<n;i++) {
    v=*hex++;
    if ((v>=0x20)&&(v<0x7f))
      *dest=v;
    else
      *dest='.';
    dest++;
  }
  *dest=0;

}




/* ****************************************** */
/* dasm1(adr)                                 */
/*                                            */
/* display a disassembly line                 */
/* ****************************************** */
unsigned int dasm1(unsigned int adr)
{
  int j, n;
  int codes[4];
  char ligne[80];

  lit(adr,3,codes);
  n=desas(adr,codes,ligne);
  printf("%04X  ",adr);
  for (j=0;j<n;j++) printf("%03X ",codes[j]);
  for (;j<3;j++) printf("    ");
  printf(" %s\n",ligne);
  adr += n;
  return(adr);
}




/* ****************************************** */
/* ch_reg(reg, s)                             */
/*                                            */
/* build a string with the content            */
/*   of a register                            */
/* ****************************************** */
void ch_reg(unsigned char *reg, char *s)
{
  int i;

  for (i=0;i<14;i++)
    s[i]=inttohex(reg[13-i]);
  s[14]=0;
  strlwr(s);
}



/* ****************************************** */
/* affiche(full)                              */
/*                                            */
/* display all or part of HP41 CPU registers  */
/* ****************************************** */
void affiche(int full)
{
  char s[20];
  int i;

  ch_reg(regA,s); printf("A=%s ",s);
  ch_reg(regB,s); printf("B=%s ",s);
  ch_reg(regC,s); printf("C=%s ",s);
  printf("\n");
  if (full) {
    ch_reg(regM,s); printf("M=%s ",s);
    ch_reg(regN,s); printf("N=%s ",s);
    printf("FO=%02x FI=%04x ",regFO, regFI);
    printf("\n");
  }

  printf("Data=%03x Per=%02x PC=%04x G=%02x",
         regData, regPer, regPC, regG);
  if (regPT==0) printf(" P=%x",regPQ[0]); else printf(" Q=%x",regPQ[1]);
  if (Carry) printf(" Cy=1"); else printf(" Cy=0");
  if (flagdec) printf(" DEC"); else printf(" HEX");
  printf(" ST=%04x ",regST);
  printf("\n");

  printf("Stack:");
  for (i=0;i<4;i++)
    printf(" %04x", retstk[i]);
  printf("\n");


  dasm1(regPC);
}


/* ****************************************** */
/* set_brkpt(adr)                             */
/*                                            */
/* set the breakpoint at address adr          */
/* ****************************************** */
void set_brkpt(int adr)
{
  if (lastaddr==0xffff) {
    lastaddr=adr;
    lastopcode=litmot(adr);
    ecritmot(adr,BRKOPC);
  }
}


/* ****************************************** */
/* res_brkpt()                                */
/*                                            */
/* reset the breakpoint                       */
/* ****************************************** */
void res_brkpt(void)
{
  if (lastaddr!=0xffff) {
    if (litmot(lastaddr)==BRKOPC) {
      ecritmot(lastaddr,lastopcode);
      lastaddr=0xffff;
    }
  }
}




/* ****************************************** */
/* view_display()                             */
/*                                            */
/* view the HP41 display content and status   */
/* ****************************************** */
void view_display(void)
{
  char buf[80];

  printf("'%s'  ",display_to_buf(buf));
  if (dspon) printf("dspon\n"); else printf("dspoff\n");
}


/* ****************************************** */
/* monitor(sst)                               */
/*                                            */
/* execute one or 256 CPU steps               */
/* ****************************************** */
int monitor(int sst)
{
  int n, res;

  if (sst) n=1; else n=256;
  res=executeNUT(n);
  if ((res==2)||(res==3)) regPC--;
  return(res);
}





/* ****************************************** */
/* checksum()                                 */
/*                                            */
/* display the ROM IDs and test the ROM       */
/*   checksum (internal and modules)          */
/* ****************************************** */
void checkrom(void)
{
  int i, j, n, m;
  char ligne[6];
  int buf[4];

  /* test ROM interne */

  /* test ROM modules */
  for (i=0;i<16;i++) {
    n=litmot(i*4096);
    m = n | litmot(i*4096+4095);
    m |= litmot(i*4096+4094);
    if (m==0) continue;
    for (j=0;j<4;j++)
      buf[j]=litmot(i*4096+4091+j);
    alpha41(ligne,buf,4);
    if (i>=3)
      printf("%X: %02d %c%c-%c%c ",i, n,ligne[3],ligne[2],ligne[1],ligne[0]);
    else
      printf("%X: SYSTEM %c ",i,ligne[3]);
    n=0xc00;
    for (j=0;j<4096;j++) {
      n+=litmot(i*4096+j);
      if (n>0xfff) {
        n &= 0xfff;
        n += 0xc01;
      }
    }
    n -= 0xc01;
    if (n==0)
      printf("OK\n");
    else
      printf("BAD\n");
  }


}

/* ****************************************** */
/* help()                                     */
/*                                            */
/* help screen                                */
/* ****************************************** */
void help(void)
{
#ifndef HPPLUS
  printf("b code        : set break conditions\n");
  printf("c             : check ROM\n");
  printf("d [ . | addr | =symb ] : disasm (.=current PC)\n");
  printf("g addr        : go, run until PC=addr\n");
  printf("j addr        : jump, set PC=addr\n");
  printf("+             : increment PC\n");
  printf("k x           : push keycode x into buffer\n");
  printf("m [ . | addr] : microcode ROM dump\n");
  printf("q             : quit\n");
  printf("r [addr]      : run emulator [break at addr]\n");
  printf("t             : trace (single step)\n");
  printf("u [ n ]       : user RAM registers dump\n");
  printf("vd            : view display buffer\n");
  printf("vk            : view key status\n");
  printf("vp            : view HPIL registers\n");
  printf("vr            : view CPU registers\n");
  printf("vt            : view timer registers\n");
#else
  printf("no help, see doc\n");  /* to save space of the portable */
#endif
}


static char nomreg[] = "TZYXLMNOPQ-abcde";  /* names of the user status registers */

/* ****************************************** */
/* commande()                                 */
/*                                            */
/* monitor command interpreter                */
/* returns 1 for cmd 'r',                     */
/*    and -1 for command 'q'                  */
/* ****************************************** */
int commande(void)
{
  char cmd[82];
  char ligne[82];
  char *tok;
  int codes[64];
  unsigned char buf[8];
  unsigned int debut, fin;
  int  ret;
  unsigned int addr;
  int i, n;

  printf("\n");
  printf("Emu41 Ready.\n");
  affiche(1);
  fin=0;
  do {
    ret=0;
    printf("Cmd: ");
    fgets(cmd,82,stdin);
    tok=strtok(cmd," ");
    tok=strtok(NULL," ");
    if (tok!=NULL) {
      if (tok[0]=='.')
        debut=regPC;
      else if (tok[0]=='=') {
        addr=ch_symb(tok);
        if (addr!= 0xffff)
          debut=addr;
        else {
          printf("%s not found\n",tok);
          cmd[0]=0;
        }
      }
      else
        sscanf(tok,"%x",&debut);
    }
    else
      debut=fin;

    switch (cmd[0]) {

      case 'b':
        breakcode=debut;
        break;

      case 'c':
        checkrom();
        break;

      case 'd':
        addr=debut;
        for (i=0;i<20;i++) {
          addr=dasm1(addr);
        }
        fin=addr;
        break;

      case 'g':
        set_brkpt(debut);
        do {
          ret=monitor(0);
          if (kbhit()) {
            if (getch()==27) ret=-2;
          }
        } while (ret==0);
        res_brkpt();
        affiche(0);
        ret=0;
        break;

      case 'i':
        for (i=0;i<16;i++) {
         printf("%2d: %d %d\n",i,typmod[i],fhram[i]);
        }
        break;

      case 'j':
        regPC=debut;
        affiche(0);
        break;

      case '+':
        regPC++;
        break;

      case 'k':
        push_key(debut);
        break;

      case 'l':
        break;

      case 'm':
        fin=debut+128;
        addr=debut;
        for (n=0;n<16;n++) {
          lit(addr,8,codes);
          printf("%04X  ",addr);
          for (i=0;i<8;i++)
            printf("%03X ",codes[i]);
          alpha41(ligne,codes,8);
          ligne[8]=0;
          printf("  %s",ligne);
          alphaASCII(ligne,codes,8);
          ligne[8]=0;
          printf("  %s\n",ligne);
          addr += 8;
        }
        fin=addr;
        break;

      case 'q':
        ret=-1;
        break;

      case 'r':
        if (tok!=NULL) set_brkpt(debut);
        ret=simulator();
        res_brkpt();
        if (ret!=1) {
          printf("\n");
          affiche(0);
          ret=0;
        }
        break;

      case 's':
        break;

      case 't':
        do {
          monitor(1);
          affiche(0);
        } while (getch()!=27);
        break;

      case 'u':
        fin=debut+16;
        for (addr=debut;addr<fin;addr++) {
          memcpy(buf,&espaceRAM[addr*8],8);
          printf("%03X",addr);
          if (addr<16) printf("(%c)",nomreg[addr]);
          printf(":  ");
          if (buf[0]==0) {
            for (i=7;i>=1;i--)
              printf("%02X ",buf[i]);
          }
          printf("\n");
        }
        fin=addr;
        break;

      case 'v':
        switch (cmd[1]) {
          case 'd':
            view_display();
            break;
          case 'k':
            printf("key flag=%d  key buffer=%d\n",flagKB,regK);
            break;
          case 'p':
            printf("hpil reg: ");
            for (i=0;i<8;i++) printf(" %02x",hpil_reg[i]);
            printf("  (%02x)\n",(hpil_reg[8]&0xe0)|flgenb);
            break;
          case 'r':
            affiche(1);
            break;
#ifndef HPPLUS
          case 't':
            printf("clock    A = ");
            for (i=13;i>=0;i--) printf("%x",clock_reg[0][i]);
            printf("  B = ");
            for (i=13;i>=0;i--) printf("%x",clock_reg[1][i]);
            printf("\n");
            printf("alarm    A = ");
            for (i=13;i>=0;i--) printf("%x",alarm_reg[0][i]);
            printf("  B = ");
            for (i=13;i>=0;i--) printf("%x",alarm_reg[1][i]);
            printf("\n");
            printf("scratch  A = ");
            for (i=13;i>=0;i--) printf("%x",scratch_reg[0][i]);
            printf("  B = ");
            for (i=13;i>=0;i--) printf("%x",scratch_reg[1][i]);
            printf("\n");
            printf("accuracy = ");
            for (i=3;i>=0;i--) printf("%x",accuracy_factor[i]);
            printf("  interval = ");
            for (i=4;i>=0;i--) printf("%x",interval_timer[i]);
            printf("  status = ");
            for (i=3;i>=0;i--) printf("%x",timer_status[i]);
            printf("\n");
            break;
#endif
          default:
            printf("?\n");
        }
        break;

      case 'w':
        break;

      case 'x':
        break;

      case '?':
        help();
        break;

      default:
        printf("?\n");
    }
  } while (ret==0);

  return(ret);
}
