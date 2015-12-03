/* hpil.h */

void init_hpil(void);

void hpil_wr(int reg, int n);
int  hpil_rd(int reg);

/* hpil_reg[] : 0-7=R0-R7, 8=R1W */
GLOBAL unsigned char hpil_reg[10];
GLOBAL char flgenb;

