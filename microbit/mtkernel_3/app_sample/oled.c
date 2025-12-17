#include <tk/tkernel.h>
#include <tm/tmonitor.h>
#include "oled.h"

W DisplayON = 0x0F,
ClearDisplay = 0x01,
ReturnHome = 0x02;

//-------- OLED
void writeData(UB t_data)
{
	write_reg(OLED_ADRS,0x40,t_data);
}

void writeCommand(UB t_command)
{
	write_reg(OLED_ADRS,0x00,t_command);
}

void contrast_max()
{
	writeCommand(0x2a);//RE=1
	writeCommand(0x79);//SD=1
	writeCommand(0x81);//コントラストセット
	writeCommand(0xFF);//輝度ＭＡＸ
	writeCommand(0x78);//SD を０にもどす
	writeCommand(0x28);//2C=高文字　28=ノーマル
}

void init_oled()
{
	tk_dly_tsk(100);
	writeCommand(ClearDisplay); // Clear Display
	tk_dly_tsk(20);
	writeCommand(ReturnHome); // ReturnHome
	tk_dly_tsk(2);
	writeCommand(DisplayON); // Send Display on command
	tk_dly_tsk(2);
	writeCommand(ClearDisplay); // Clear Display
	tk_dly_tsk(20);
}

