/* timer.h */

void timer_rd_n (int n);
void timer_wr_n (int n);
void timer_wr (void);

void init_timer(void);


GLOBAL unsigned char clock_reg [2][14];
GLOBAL unsigned char alarm_reg [2][14];
GLOBAL unsigned char scratch_reg[2][14];
GLOBAL unsigned char timer_status[4];
GLOBAL unsigned char accuracy_factor[4];
GLOBAL unsigned char interval_timer[5];
