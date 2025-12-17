#ifndef _OLED_H
#define _OLED_H

#include <tk/typedef.h>

//-------- OLED ---------------------------------------------
#define OLED_ADRS 0x3C //SA0=L(SA0=H の場合は 0x3D)



void writeData(UB t_data);
void writeCommand(UB t_command);

#endif //_OLED_H

