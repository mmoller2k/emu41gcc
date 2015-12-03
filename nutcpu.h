/* ****************************************** */
/* Emu41: HP-41C software emulator for PC     */
/*                                            */
/* nutcpu.h   NUT CPU simulator definitions   */
/*                                            */
/* Version 2.4, J-F Garnier 1997-2006         */
/* ****************************************** */



/* registrer definition */
GLOBAL unsigned char regA[14];
GLOBAL unsigned char regB[14];
GLOBAL unsigned char regC[14];
GLOBAL unsigned char regM[14];
GLOBAL unsigned char regN[14];
GLOBAL unsigned int  retstk[4];
GLOBAL unsigned int  regPC;
GLOBAL unsigned int  regST;
GLOBAL unsigned char regPQ[2];
GLOBAL unsigned char regG;
GLOBAL unsigned char Carry;
GLOBAL unsigned char regK;    /* key, read by C=KEYS */
GLOBAL unsigned char regFO;   /* output flags (not used) */
GLOBAL unsigned int  regFI;   /* input flags (used by HPIL) */

/* internal registers */
GLOBAL char regPT;    /* active nibble pointer (0=P, 1=Q) */
GLOBAL char flagKB;   /* flag key available, read by CHKKB */
GLOBAL char flagdec;  /* decimal mode */
GLOBAL int  regData;  /* selected data register */
GLOBAL int  regPer;   /* selected peripheral register (wand, display, cardreader, clock) */

GLOBAL char flagKey;       /* virtual key state (key state machine) 1=one key down*/
GLOBAL char mode_printer;  /* HP82143 printer mode (0=OFF, 1=NORM, 2=MAN, 3=TRACE) */
GLOBAL char flagPrter;     /* HP82143 printer: data available */
GLOBAL char flagPrx;       /* HP82143 printer: PRINT key down */
GLOBAL char flagAdv;       /* HP82143 printer: ADVANCE key down */
GLOBAL int  cptKey;        /* counter for key state machine */

GLOBAL char smartp;    /* smart peripheral flag */   
GLOBAL char breakcode; /* code for conditional breakpoint */
GLOBAL int  selper;    /* selected smart peripheral (HPIL, HP82143) */

GLOBAL int  cptinstr;  /* instruction counter */
GLOBAL char fjmp;      /* jump flag */

/* display */
GLOBAL char dspon;     /* LCD on */
/*GLOBAL int fslow;*/
GLOBAL int facces_dsp; /* flag access to LCD (LCD change) */
GLOBAL char fdsp;      /* flag display needs update */

/* keyboard */
GLOBAL char keybuffer[8];  /* key buffer (8 keys max) */
GLOBAL int lgkeybuf;       /* length of keybuffer (number of keys) */


/* memory areas */
GLOBAL unsigned char espaceRAM[8200];  /* 1024 RAM registers (8 bytes/reg) + 1 extra */
//GLOBAL unsigned short tabpage[16];     /* table of active page segments   */
short *tabpage[16];
GLOBAL unsigned char typmod[16];       /* module types */
  /* 1=ROM, 2=CXTIME, 3=Advantage, 4=HEPAX ROM, 5=HEPAX RAM, 6=IR printer, 7=MLDL RAM,
     8=W&W ROMBOX, 9=W&W RAMBOX, 10=ES ROM Storage Unit, 11=ES RAM Storage Unit */
//GLOBAL unsigned short tabbank[16][4];  /* table of bank segments (4 possible bank / page) */
short *tabbank[16][4];
GLOBAL unsigned char fhram[16];        /* flags Hepax protected ram */

/* hp-il devices */
GLOBAL int nbdev;           /* number of devices */
GLOBAL int tabdev[32];      /* device table */
/* 1:VIDEO, 2:HDRIVE1, 3:FDRIVE1, 4:DOS, 5:XIL, 6:LPRTER1, 7:HDRIVE2, 8:SERIAL1, 9:SERIAL2 */

/* function prototypes  */
int executeNUT(int n);
/* value returned by executeNUT :
   0 : OK
   1:  POWOFF
   2:  invalid opcode
   3:  breakpoint
*/

