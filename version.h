/* version.h */

#ifdef VERS_SLOW
/* Light version, for HP95/100/200LX, HP Portable, or Classic Vectra 286 */

#ifdef HPLX

#undef XIL
/*             1234567890123456789012345678901234 */
#define USER1 " HP-LX serie version (F1 to exit) "
#define USER2 "   For more information, visit:   "
#define USER3 " http://membres.lycos.fr/jeffcalc "
#define SIG   " PRT:LX1"
#define WSUM -13586

#endif

/* ------------------------------------------------- */

#ifdef HPPLUS

#define XIL
/*             1234567890123456789012345678901234 */
#define USER1 " HP Portable version (F1 to exit) "
#define USER2 "   For more information, visit:   "
#define USER3 " http://membres.lycos.fr/jeffcalc "
#define SIG   " PRT:PP1"
#define WSUM 0

#endif

/* ------------------------------------------------- */

#ifndef HPLX
#ifndef HPPLUS

#define XIL

#define USER1 "    Emu41LT  Light Version        "
#define USER2 "                                  "
#define USER3 ""
#define SIG   " PRT:LT1"
#define WSUM -8189

#endif
#endif

/* ------------------------------------------------- */

#else
/* standard PC version (XIL included now) */

#define XIL

/*             1234567890123456789012345678901234 */
#define USER1 "    Free version  (F1 to exit)    "
#define USER2 "   For more information, visit:   "
#define USER3 " http://membres.lycos.fr/jeffcalc "


#endif


