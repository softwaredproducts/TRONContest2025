#ifndef _BME280_H
#define _BME280_H

#include <tk/typedef.h>

//-------- BME280 -------------------------------------------
#define IICADR_BME  0x76

void bme280_task(INT stacd, void *exinf);

#endif //_BME280_H
