#include <tk/tkernel.h>
#include <tm/tmonitor.h>
#include "ultrasonic.h"
#include "oled.h"
#include "common_func.h"

//-------- ultrasonic
UW timer1_get_us(void) {
    *(volatile UW *)0x40009040 = 1;  // CC[0]に現在値を格納
    return *(volatile UW *)0x40009540;          // CC[0]を読む
}

/* wait n ms */
void wait_usec(UW rv_dlytim){
	for (UW cnt = 0;cnt < rv_dlytim; cnt ++){
		;
	}
}

// timer initialize
void timer1_init(void) {
    *(volatile UW *)0x40009004 = 1;
    *(volatile UW *)0x40009504 = 0;   // Timerモード
    *(volatile UW *)0x40009508 = 3;   // 32-bit
    *(volatile UW *)0x40009510 = 4;   // 16MHz / (2^4) = 1MHz → 1us刻み
    *(volatile UW *)0x4000900C = 1;
    *(volatile UW *)0x40009000 = 1;
}

void gpio_init(void)
{
	out_w(GPIO(P0, PIN_CNF(12)),0x4);
	out_w(GPIO(P1, PIN_CNF(GPIO_P16)),0x00000001);
	out_w(GPIO(P1, DIRSET), 1 << GPIO_P16);
}

void build_message_ultrasonic(char* moji, int duration, int distance_cm) {
    char durationStr[12], distance_cmStr[12];

    cf_itoa(duration, durationStr);
    cf_itoa(distance_cm, distance_cmStr);

    cf_strcpy(moji, "Distance:");
    cf_strcat(moji, distance_cmStr);
    cf_strcat(moji, "cm    ");
}

// Ultrasonicタスク
void ultrasonic_task(INT stacd, void *exinf)
{

    SYSTIM start_time, end_time;
    UW duration;
    UW distance_cm;
    UW t0,t1;
    UW x0, s[2];
    B moji[21];

	// ultrasonic
	timer1_init();
	gpio_init();

	out_w(GPIO(P0, PIN_CNF(12)),0x4);
	out_w(GPIO(P1, PIN_CNF(GPIO_P16)),0x00000001);
	out_w(GPIO(P1, DIRSET), 1 << GPIO_P16);

	for (;;) {				// 約300msごとに永久に繰り返し

		UINT intsts;
		DI(intsts);

    	out_w(GPIO(P1,OUTSET),1 << GPIO_P16);
        //tk_dly_tsk(500);
        wait_usec(1000);
        out_w(GPIO(P1,OUTCLR),1 << GPIO_P16);


        while(1){
        	x0 = in_w(GPIO(P0,IN)) & (1 << 12);
        	if(x0 != 0){
        		break;
        	}
        }
        t0 = timer1_get_us();

        // Echoピンの立ち下がりを待つ
        while(1){
        	x0 = in_w(GPIO(P0,IN)) & (1 << 12);
        	//tm_printf((UB*)"-%d",x0);
        	if(x0 == 0){
        		break;
        	}
        }
        t1 = timer1_get_us();
        EI(intsts);

        duration = t1 - t0;
        distance_cm = (duration * 1715) / 100000;

        tm_printf((UB*)"duration=%d distance_cm=%d\n", duration, distance_cm);
        build_message_ultrasonic(moji,duration,distance_cm);

		writeCommand(0x20+0x80);
		for(B i = 0;i < 20;i++){
			writeData(moji[i]);
		}
		tk_dly_tsk(300);					// 300msのディレイ
	}
}
