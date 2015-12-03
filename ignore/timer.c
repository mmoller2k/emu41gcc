/* ****************************************** */
/* Emu41: HP-41C software emulator for PC     */
/*                                            */
/* timer.c   Timer chip emulation module      */
/*                                            */
/* Version 2.5, J-F Garnier 1997-2007         */
/* ****************************************** */

#include <stdio.h>
#include <mem.h>
#include <dos.h>
#include <time.h>
#include <sys\timeb.h>

#define GLOBAL extern
#include "nutcpu.h"
#undef GLOBAL

#define GLOBAL
#include "timer.h"
#undef GLOBAL


static int timer_reg_sel; /* selected timer (A or B) */
/* local variables for real-time count: */
static long lastticks;
static long lastticks2;


/* status bits (def no more used, but kept for documentation) : */
#define ALMA 0
#define DTZA 1
#define ALMB 2
#define DTZB 3
#define DTZIT 4
#define PUS 5
#define CKAEN 6
#define CKBEN 7
#define ALAEN 8
#define ALBEN 9
#define ITEN 10
#define TESTA 11
#define TESTB 12




/* ****************************************** */
/* incr_timer(reg, d)                         */
/*                                            */
/* increment one timer register by count d    */
/* ****************************************** */
static void incr_timer(char *regdest, long d)
{
  int i, n, cy;

  cy=0;
  for (i=0;i<14;i++) {
    n = *regdest + (d%10) + cy;
    if (n>=10) {
      cy=1;
      n -=10;
    }
    else
      cy=0;
    *regdest++ = n;
    d /= 10;
  }


}


/* ****************************************** */
/* update_timer()                             */
/*                                            */
/* update the timers based on PC RTC          */
/* ****************************************** */
void update_timer(void)
{
  long t;
  long d;

  t=clock();
#if __TURBOC__ < 0x400
  t *= 55;  /* in ms */
#endif

  if (timer_status[1]&4) {
    d=t-lastticks;
    d /= 10;     /* in 1/100 s */
    lastticks += d*10;
    incr_timer(clock_reg[0],d);
  }
  if (timer_status[1]&8) {
    d=t-lastticks2;
    d /= 10;
    lastticks2 += d*10;
    incr_timer(clock_reg[1],d);
  }
}


/* ****************************************** */
/* setup_timer()                              */
/*                                            */
/* setup the timers based on PC RTC           */
/* ****************************************** */
static void setup_timer(void)
{
  time_t tm;
  struct timeb buf;
  int i, n, cy;
  char *regdest;

  regdest=clock_reg[0];
  /* offset between 1900, jan, 1 (base for HP-41)
                and 1970, jan, 1 (base for time()) */
  tm = (25567L * 24 ) * 36 ;     /* 25567 days, converted to 100 x seconds  */
  for (i=4;i<14;i++) {
    regdest[i] = tm%10;
    tm /= 10;
  }

  tzset();
  lastticks=clock();    /* origin of HP41 ticks */
#if __TURBOC__ < 0x400
  lastticks *= 55;  /* en ms */
#endif
  lastticks2=lastticks;
  ftime(&buf);
  tm = buf.time - buf.timezone*60;
  tm += buf.dstflag*3600;  /* JFG 31/08/02 */
#if __TURBOC__ >= 0x400
/*  tm += _daylight*3600; /* correction for daylight saving */
#endif

  regdest=clock_reg[0];
  cy=0;
  for (i=2;i<14;i++) {
    n = regdest[i] + (tm%10) + cy;
    if (n>=10) {
      cy=1;
      n -=10;
    }
    else
      cy=0;
    regdest[i] = n;
    tm /= 10;
  }

  memcpy(scratch_reg[0],clock_reg[0],14);

}




/* ****************************************** */
/* timer_rd_n(n)                              */
/*                                            */
/* timer chip read opcodes                    */
/* initially based on NSIM05 from Eric Smith  */
/* ****************************************** */
void timer_rd_n (int n)
{
  switch (n) {
    case 0x0: /* 038: RTIME */
    case 0x1: /* 078: RTIMEST */
      update_timer();
      memcpy (regC, clock_reg [timer_reg_sel], 14);
      break;
    case 0x2: /* 0b8: RALM */
      memcpy (regC, alarm_reg [timer_reg_sel], 14);
      break;
    case 0x3: /* 0f8: RSTS */
      if (timer_reg_sel==0) {
        memcpy (regC, timer_status, 4);
        memset (&regC[4], 0, 10);
      }
      else {
        regC[0]=0;
        memcpy (&regC[1], accuracy_factor , 4);
        memset (&regC[5], 0, 9);
      }
      break;
    case 0x4: /* 138: RSCR */
      memcpy (regC, scratch_reg [timer_reg_sel], 14);
      break;
    case 0x5: /* 178: RINT */
      memcpy (regC, interval_timer, 5);
      memset (&regC[5], 0, 9);
      break;
    }
}

/* ****************************************** */
/* timer_wr_n(n)                              */
/*                                            */
/* timer chip write opcodes                   */
/* initially based on NSIM05 from Eric Smith  */
/* ****************************************** */
void timer_wr_n (int n)
{
  switch (n) {
    case 0x0: /* 028: WTIME   write C to clock register */
    case 0x1: /* 068: WTIME-  write C to clock register and set to decr */
      memcpy (clock_reg [timer_reg_sel], regC, 14);
      break;
    case 0x2: /* 0a8: WALM    write C to alarm register */
      memcpy (alarm_reg [timer_reg_sel], regC, 14);
      break;
    case 0x3: /* 0e8: WSTS */
      if (timer_reg_sel==0) {
        timer_status[0] &= regC[0];
        timer_status[1] &= (regC[1]|0x0c);
      }
      else {
        memcpy ( accuracy_factor , &regC[1],4);
        accuracy_factor[3] &= 1;
      }
      break;
    case 0x4: /* 128: WSCR    write C to scratch register */
      memcpy (scratch_reg[timer_reg_sel], regC, 14);
      break;
    case 0x5: /* 168: WINTST  write C to interval timer and start */
      memcpy (interval_timer, regC, 5);
      timer_status[2] |= 4;  /* ITEN = 1 */
      break;
    case 0x7: /* 1e8: STPINT  stop interval timer */
      timer_status[2] &= ~4;  /* ITEN = 0 */
      break;
    case 0x8: /* 228:  */
      break;
    case 0x9: /* 268: */
      break;
    case 0xa: /* 2a8: ALMOFF */
      if (timer_reg_sel==0)
        timer_status[2] &= ~1;  /* ALAEN = 0 */
      else
        timer_status[2] &= ~2;  /* ALBEN = 0 */
      break;
    case 0xb: /* 2e8: ALMON */
      if (timer_reg_sel==0)
        timer_status[2] |= 1;  /* ALAEN = 1 */
      else
        timer_status[2] |= 2;  /* ALBEN = 1 */
      break;
    case 0xc: /* 328: STOPC */
      update_timer();
      if (timer_reg_sel==0)
        timer_status[1] &= ~4;  /* CKAEN = 0 */
      else
        timer_status[1] &= ~8;  /* CKBEN = 0 */
      break;
    case 0xd: /* 368: STARTC */
      if (timer_reg_sel==0) {
        timer_status[1] |= 4;  /* CKAEN = 1 */
      }
      else {
        timer_status[1] |= 8;  /* CKBEN = 1 */
        lastticks2=clock();   /* origin of HP41 ticks */
#if __TURBOC__ < 0x400
        lastticks2 *= 55;  /* in ms */
#endif
      }
      break;
    case 0xe: /* 3a8: TIMER=B */
      timer_reg_sel = 1;
      break;
    case 0xf: /* 3e8: TIMER=A */
      timer_reg_sel = 0;
      break;
    }
}


/* ****************************************** */
/* init_timer()                               */
/*                                            */
/* init the timer registers,                  */
/*   and set current time                     */
/* ****************************************** */
void init_timer (void)
{
  int t;

  for (t = 0; t < 2; t++) {
    memset(clock_reg[t],0,14);
    memset(alarm_reg[t],0,14);
    memset(scratch_reg[t],0,14);
  }
  memset(accuracy_factor,0,4);
  memset(interval_timer,0,5);
  memset(timer_status,0,4);
  timer_reg_sel=0;

  timer_status[1]=4; /* CKAEN=1 */
  timer_status[2]=1; /* ALAEN=1 */
  memset(&alarm_reg[1][3],9,10);
  scratch_reg[1][1]=4;  /* CLK24 */

  setup_timer();

}




