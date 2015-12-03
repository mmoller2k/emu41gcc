/* ****************************************** */
/* Emu41: HP-41C software emulator for PC     */
/*                                            */
/* userpc.c   user interface for emu41        */
/* system dependant (PC version)              */
/*                                            */
/* Version 2.5, J-F Garnier 1997-2007         */
/* ****************************************** */

/* Conditional compilation:
  none
*/

#include <stdio.h>
#include <conio.h>
#include <dos.h>


/* DISPLAY variables */
static int curx, cury;      /* cursor position */
static int nlines=24;       /* number of lines */
static int ncols=80;        /* number of cols (not used) */
static char fready;         /* display ready */
static char fesc=0;         /* ESC sequence on going */
static char fcur=0;         /* cursor active */
static char fctyp=1;        /* cursor type */
static char fupdate=0;      /* last screen operation (1:LCD, 2:video display)

/* Welcome strings */
extern char *Welcome1;
extern char *Welcome2;
extern char *Welcome3;
extern char *Welcome4;


/* ********* utilities ********** */

/* specific for Borland Turbo C 2.0 */
#if __TURBOC__ < 0x200
#define _NOCURSOR 0x2000
#define _NORMALCURSOR 0x607
#define _SOLIDCURSOR 0x007
void _setcursortype(int n)
{
  _AX=0x0100;
  _CX=n;
  geninterrupt(0x10);
}
#endif

/* ****************************************** */
/* restore_cursor()                           */
/*                                            */
/* restore the normal cursor                  */
/* ****************************************** */
void restore_cursor(void)
{
  _setcursortype(_NORMALCURSOR);
}

/* ****************************************** */
/* cursor_off()                               */
/*                                            */
/* hide the cursor                            */
/* ****************************************** */
void cursor_off(void)
{
  _setcursortype(_NOCURSOR);
}



/* ********* keyboard support ********** */


/* ****************************************** */
/* tstkey()                                   */
/*                                            */
/* Test if one PC key available               */
/* returns 1 if yes                           */
/* ****************************************** */
int tstkey(void)
{
  return(kbhit());
}


/* ****************************************** */
/* readkey()                                  */
/*                                            */
/* Read one PC key available                  */
/* returns -1 if none                         */
/* ****************************************** */
int readkey(void)
{
  if (kbhit())
    return(getch());
  else
    return(-1);
}

/* ****************************************** */
/* waitforkey()                               */
/*                                            */
/* Wait for one PC key available              */
/* returns the keycode                        */
/* ****************************************** */
int waitforkey(void)
{
  return(getch());
}


/* ********* screen functions ********** */

/* ****************************************** */
/* fen_aide()                                 */
/*                                            */
/* Draw the help window                       */
/* ****************************************** */
void fen_aide(int mode_printer)
{
  window(1,1,80,nlines);
  textattr(WHITE);
  clrscr();

  /* draw the welcome strings */
  textbackground(BLUE);
  textcolor(WHITE);
  gotoxy(1,1);
  cputs(Welcome1);
  gotoxy(3,3);
  cputs(Welcome2);
  gotoxy(3,4);
  cputs(Welcome3);
  gotoxy(3,5);
  cputs(Welcome4);

  /* bottom help line */
  gotoxy(1,nlines);
  /*     12345678901234567890123456789012345678901234567890123456789012345678901234567890 */
  cputs(" TAB:speed  End:R/S  PageUp:BST  PageDown:SST  <-:CLX  ->:X<>Y  ^:SHIFT  v:RDN ");


  /* right window with keyboard mapping */
  window(42,2,80,nlines-4);
  gotoxy(1,2);
  textbackground(BLUE);
  /*      1234567890123456789012345678901234567890 */
  textcolor(WHITE);
  cputs("        K E Y    M A P P I N G      \n\r");
  cputs("                                    \n\r");

  textcolor(YELLOW);
  cputs("   S-     Y^X    X^2    10^X   e^X  \n\r");
  textcolor(WHITE);
  cputs(" A:S+   B:1/X  C:SQRT D:LOG  E:LN   \n\r");
  cputs("                                    \n\r");

  textcolor(YELLOW);
  cputs("   CLS    %      ASIN   ACOS   ATAN \n\r");
  textcolor(WHITE);
  cputs(" F:X<>Y G:RDN  H:SIN  I:COS  J:TAN  \n\r");
  cputs("                                    \n\r");

  textcolor(YELLOW);
  cputs("          ASN    LBL    GTO         \n\r");
  textcolor(WHITE);
  cputs("        K:XEQ  L:STO  M:RCL         \n\r");
  cputs("                                    \n\r");

  textcolor(YELLOW);
  cputs("     CATALOG     ISG    RTN         \n\r");
  textcolor(WHITE);
  cputs("   N:ENTER     O:CHS  P:EEX         \n\r");
  cputs("                                    \n\r");

  textcolor(YELLOW);
  cputs(" -:X=Y?  7:SF    8:CF     9:FS? |   \n\r");
  if (nlines>25)
    cputs("                                    \n\r");
  cputs(" +:X<=Y? 4:BEEP  5:P-R    6:R-P |with\n\r");
  if (nlines>25)
    cputs("                                    \n\r");
  cputs(" *:X>Y?  1:FIX   2:SCI    3:ENG |SHIFT\n\r");
  if (nlines>25)
    cputs("                                    \n\r");
  cputs(" /:X=0?  0:PI    .:LASTX        |   \n\r");
  if (nlines>25)
    cputs("                                    ");

  textcolor(WHITE);

  if (nlines>25) {
    gotoxy(2,16);  cputs("-:"); gotoxy(10,16); cputs("7:");
    gotoxy(18,16); cputs("8:"); gotoxy(27,16); cputs("9:");
    gotoxy(2,18);  cputs("+:"); gotoxy(10,18); cputs("4:");
    gotoxy(18,18); cputs("5:"); gotoxy(27,18); cputs("6:");
    gotoxy(2,20);  cputs("*:"); gotoxy(10,20); cputs("1:");
    gotoxy(18,20); cputs("2:"); gotoxy(27,20); cputs("3:");
    gotoxy(2,22);  cputs("/:"); gotoxy(10,22); cputs("0:");
    gotoxy(18,22); cputs(".:");
  }

  /*  function key labels */
  window(1,nlines-2,80,nlines-1);
  textattr(WHITE);
  clrscr();
  window(1,1,80,nlines);
  textattr(LIGHTGRAY<<4);

  gotoxy( 2,nlines-2); cputs(" f1  ");
  gotoxy( 8,nlines-2); cputs(" f2  ");
  gotoxy(14,nlines-2); cputs(" f3  ");
  gotoxy(20,nlines-2); cputs(" f4  ");

  gotoxy(29,nlines-2); cputs(" f5  ");
  gotoxy(35,nlines-2); cputs(" f6  ");
  gotoxy(41,nlines-2); cputs(" f7  ");
  gotoxy(47,nlines-2); cputs(" f8  ");

  gotoxy(56,nlines-2); cputs(" f9  ");
  gotoxy(62,nlines-2); cputs(" f10 ");
  gotoxy(68,nlines-2); cputs(" f11 ");
  gotoxy(74,nlines-2); cputs(" f12 ");

  gotoxy( 2,nlines-1); cputs(" ON  ");
  gotoxy( 8,nlines-1); cputs("USER ");
  gotoxy(14,nlines-1); cputs("PRGM ");
  gotoxy(20,nlines-1); cputs("ALPHA");

  gotoxy(29,nlines-1); cputs("SHIFT");
  gotoxy(35,nlines-1); cputs(" SST ");
  gotoxy(41,nlines-1); cputs(" R/S ");
  gotoxy(47,nlines-1); cputs(" XEQ ");

  if (mode_printer>=0) {
    /* if HP82143 printer emulation installed, then ... */
    gotoxy(56,nlines-1); cputs("MODE ");
    gotoxy(62,nlines-1); cputs(" PRX ");
    gotoxy(68,nlines-1); cputs(" ADV ");
  }
  else {
    gotoxy(56,nlines-1); cputs("     ");
    gotoxy(62,nlines-1); cputs("     ");
    gotoxy(68,nlines-1); cputs("     ");
  }
  /* F12 is a reset key */
  gotoxy(74,nlines-1); cputs("reset");

  /* f5 is the SHIFT key to be drawn in yellow ... */
  textattr((LIGHTGRAY<<4)+YELLOW);
  gotoxy(29,nlines-2); cputs(" f5  ");
  gotoxy(29,nlines-1); cputs("SHIFT");

  window(1,1,80,nlines);
  textattr(WHITE);
  _setcursortype(_NORMALCURSOR);
  gotoxy(1,nlines-4);
/*       12345678901234567890123456789012345678 */
  cputs("                         "); /* clear LCD line */
  if (mode_printer>=0) {
    /* if HP82143 printer emulation installed, display printer mode */
    gotoxy(32,nlines-4);
    cputs("PRT:MAN");
  }
  gotoxy(1,nlines-3);
  cputs("                      ");  /* clear announciator line */

  /* init display cursor position on bottom of screen */
  curx=1;
  cury=nlines-6;
}



/* ****************************************** */
/* prepa_ecran()                              */
/*                                            */
/* Prepare the screen (ecran in french)       */
/* mode _printer is a flag set if HP82143     */
/*   printer emulation is installed           */
/* ****************************************** */
void prepa_ecran(int mode_printer)
{
  int i;

  /* find out the number of lines in present display mode */
  gotoxy(1,24);  /* go to line 24 */
  cputs("\n\n\n\n");  
  nlines=wherey();
  if (nlines>25) {
    for (i=0;i<8;i++) cputs("\n\n\n\n");
    nlines=wherey();  /* should be 43 or 50 ... */
  }

  /* drax the help window */
  fen_aide(mode_printer);
  fready=1;  /* emulated video display ready */
  fesc=0;    /* no on going esc sequence */
  fupdate=0; /* display update done */
}


/* ****************************************** */
/* fin_ecran()                                */
/*                                            */
/* End the emulated video display             */
/* ****************************************** */
void fin_ecran(void)
{
  window(1,1,80,nlines);
  textattr(WHITE);
  gotoxy(1,nlines);
  _setcursortype(_NORMALCURSOR);
  cputs(" \r\n");
  fready=0;   /* emulated video display not ready */
}




/* ****************************************** */
/* update_cursor()                            */
/*                                            */
/* update the cursor type in emulated video   */
/*   display                                  */
/* ****************************************** */
void update_cursor(void)
{
  if (fcur==0) {
    _setcursortype(_NOCURSOR);  /* cursor off */
  }
  else {
    if (fctyp==2)
      _setcursortype(_NORMALCURSOR);  /* insert cursor */
    else
      _setcursortype(_SOLIDCURSOR);   /* replace cursor */
  }
}






/* ********* LCD emulation support ********** */


/* ****************************************** */
/* update_display()                           */
/*                                            */
/* update the LCD                             */
/* with display (buf1) and                    */
/*   announciator (buf2) data                 */
/* ****************************************** */
void update_display(char *buf1, char *buf2)
{
  if (fupdate!=1) {
    /* if previous access was not for LCD data, init window */
    window(1,1,80,nlines);
    textattr(WHITE);
    _setcursortype(_NOCURSOR);
    gotoxy(1,nlines-4);
    fupdate=1;
  }
  if (buf1!=NULL) {
    gotoxy(2,nlines-4);
    cputs(buf1);
  }
  if (buf2!=NULL) {
    gotoxy(1,nlines-3);
    cputs(buf2);
  }
  if (fcur) {
    /* if cursor active in video screen window, restore it */
    window(1,2,40,nlines-5);
    textattr(BLACK*16+LIGHTGREEN);
    gotoxy(curx,cury);
    update_cursor();
    fupdate=2;
  }
}

/* ****************************************** */
/* aff_mode_printer()                         */
/*                                            */
/* display the printer mode                   */
/* ****************************************** */
void aff_mode_printer(int mode)
{
  gotoxy(36,nlines-4);
  switch (mode) {
    case 0: cputs("MAN"); break;
    case 1: cputs("NOR"); break;
    case 2: cputs("TRA"); break;
    case 3: cputs("OFF"); break;
    default:cputs("   ");
  }
}

/* ****************************************** */
/* aff_mode_emu()                             */
/*                                            */
/* display the emulator state                 */
/* ****************************************** */
void aff_mode_emu(char c)
{
  gotoxy(30,nlines-4);
  putch(c);
}




/* ********* display emulation support ********** */

/* ****************************************** */
/* clrdsp()                                   */
/*                                            */
/* clear the display area,and init state      */
/* ****************************************** */
void clrdsp(void)
{
  window(1,2,40,nlines-5);
  textattr(BLACK*16+LIGHTGREEN);
  clrscr();
  curx=1;
  cury=1;
  gotoxy(curx,cury);
  fesc=0;
  fctyp=1;
  fcur=0;
  fupdate=2;
  update_cursor();
}

/* ****************************************** */
/* disp_string()                              */
/*                                            */
/* display a string in the display area       */
/*  if display area not initialized,          */
/*    do a simple printf (startup/end msgs)   */
/* ****************************************** */
void disp_string(char *s)
{
  if (!fready)
    printf("%s",s);
  else {
    if (fupdate!=2) {
      /* if previous access was not for display data, init window */
      window(1,2,40,nlines-5);
      textattr(BLACK*16+LIGHTGREEN);
      gotoxy(curx,cury);
      update_cursor();
      fupdate=2;
    }
    cputs(s);
    curx=wherex();
    cury=wherey();
  }
}


/* ****************************************** */
/* disp_char()                                */
/*                                            */
/* send a character to the display area       */
/*   manage the ESC sequences                 */
/*    (HP82163 compatible)                    */
/*   convert special HP41 characters          */
/* ****************************************** */
void disp_char(int n)
{
  int i;

  if (fupdate!=2) {
    /* if previous access was not for display data, init window */
    window(1,2,40,nlines-5);
    textattr(BLACK*16+LIGHTGREEN);
    gotoxy(curx,cury);
    update_cursor();
    fupdate=2;
  }

  switch (fesc) {

    case 0:
      if (n==8) {
        /* for Paname compatibility */
        if (curx>1) curx--;
        else {
          if (cury>1) {
            cury--;
            curx=40;
          }
        }
        gotoxy(curx, cury);
      }
      else if (n==27)
        fesc=1;         /* ESC sequence */
      else {
        switch (n) {
          /* convert special HP41 characters to regular ASCII */
          case   0: n='*'; break;
          case  12: n='u'; break;
          case  28: n='s'; break;
          case  29: n='#'; break;
          case 124: n='a'; break;
          case 127: n='`'; break;
          case 10:
          case 13: break;
          default:
           if ((n<32)||(n>127))  n='.';  /* non-printable characters */
        }
        gotoxy(curx, cury);
        putch(n);
        curx=wherex();
        cury=wherey();
      }
      break;

    case 1:
      fesc=0;
      switch (n) {
        case 'A': if (cury>1) cury--; break; /* csrup */
        case 'B': if (cury<(nlines-5)) cury++; break; /* csrdn */
        case 'C':
          if (curx<40) curx++;
          else {
            if (cury<(nlines-5)) {
              cury++;
              curx=1;
            }
          }
          break; /* csrr */
        case 'D':
          if (curx>1) curx--;
          else {
            if (cury>1) {
              cury--;
              curx=40;
            }
          }
          break; /* csrl */
        case 'E': clrdsp(); break; /* clrdsp */
        case 'H': curx=1; cury=1; break; /* home */
        case 'J': /* clrend */
          gotoxy(curx,cury);
          for (i=curx;i<=40;i++) cputs(" ");
          break;
        case 'Q': fctyp=2; break; /* csrins */
        case 'R': fctyp=1; break; /* csrrep */
        case '<': fcur=0;  break; /* csroff */
        case '>': fcur=1;  break; /* csron */
        case '%': fesc=2;  break; /* csrpos */
      }
      gotoxy(curx,cury);
      update_cursor();
      break;

    case 2: /* curpos, x value modulo 64*/
      curx=(n&63)+1;
      fesc=3;
      break;

    case 3: /* curpos, y value modulo 64 (not compatible with HP82163) */
      cury=(n&63)+1;
      gotoxy(curx,cury);
      update_cursor();
      fesc=0;
      break;

  }

}


