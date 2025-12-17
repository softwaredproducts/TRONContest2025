#include <tk/tkernel.h>
#include <tm/tmonitor.h>
#include "bme280.h"
#include "oled.h"
#include "common_func.h"

#define MAX_RECORD_NUM 10
#define USUAL_STATE 0
#define CLIMATE_CHK 1

IMPORT INT read_reg( INT adr, INT reg );
IMPORT ER write_reg( INT adr, INT reg, UB dat );
IMPORT ER read_reg_6( INT adr, INT reg, UB dat[6] );
IMPORT ER write_reg_2(INT adr, UB dat[2]);
IMPORT ER read_reg_4(INT adr,  UB dat[2]);
IMPORT ER BytesRead(INT adr, INT reg, UB* dat, UB length);

typedef D  BME280_S64_t;
typedef W  BME280_S32_t;
typedef UW BME280_U32_t;

BME280_U32_t hum_data, temp_data,pres_data;
BME280_U32_t temperature, pressure, humidity;
BME280_S32_t t_fine;

UH dig_T1;
H  dig_T2;
H  dig_T3;
UH dig_P1;
H  dig_P2;
H  dig_P3;
H  dig_P4;
H  dig_P5;
H  dig_P6;
H  dig_P7;
H  dig_P8;
H  dig_P9;
UB dig_H1;
H  dig_H2;
UB  dig_H3;
H  dig_H4;
H  dig_H5;
B  dig_H6;

// record temperature, pressure and humidity every minute
UW temperature_every_min[MAX_RECORD_NUM];
UW pressure_every_min[MAX_RECORD_NUM];
UW humidity_every_min[MAX_RECORD_NUM];
UB curr_buf_idx_pos     = 0;

void ReadTrimmingParam()
{
	UB data[32];
	BytesRead(IICADR_BME, 0x88, data, 24);
	BytesRead(IICADR_BME, 0xA1, data+24,1);
	BytesRead(IICADR_BME, 0xE1, data+24+1,7);

	dig_T1 = (data[1] << 8) | data[0];
	dig_T2 = (data[3] << 8) | data[2];
	dig_T3 = (data[5] << 8) | data[4];
	dig_P1 = (data[7] << 8) | data[6];
	dig_P2 = (data[9] << 8) | data[8];
	dig_P3 = (data[11] << 8) | data[10];
	dig_P4 = (data[13] << 8) | data[12];
	dig_P5 = (data[15] << 8) | data[14];
	dig_P6 = (data[17] << 8) | data[16];
	dig_P7 = (data[19] << 8) | data[18];
	dig_P8 = (data[21] << 8) | data[20];
	dig_P9 = (data[23] << 8) | data[22];
	dig_H1 = data[24];
	dig_H2 = (data[26] << 8) | data[25];
	dig_H3 = data[27];
	dig_H4 = (data[28] << 4) | (0x0F & data[29]);
	dig_H5 = (data[30] << 4) | ((data[29] >> 4) & 0x0F);
	dig_H6 = data[31];
}

void readData()
{
	UB data[8];
    ER at_ret = E_OK;

	at_ret = BytesRead(IICADR_BME, 0xF7,data,8);
	// if preempt happens, it should not read the data
	if(at_ret < E_OK) return;

	pres_data = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
	temp_data = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);
	hum_data  = (data[6] << 8)  | data[7];
}

BME280_S32_t calibration_T_int32(BME280_S32_t adc_T)
{
    BME280_S32_t var1;
    BME280_S32_t var2;
    BME280_S32_t temperature;
    BME280_S32_t temperature_min = -4000;
    BME280_S32_t temperature_max = 8500;
    BME280_S32_t T;
    var1 = (adc_T / 16384.0 - dig_T1 / 1024.0) * dig_T2;
    var2 = (adc_T / 131072.0 - dig_T1 / 8192.0) * (adc_T / 131072.0 - dig_T1 / 8192.0) * dig_T3;
    t_fine = var1 + var2;
    temperature = t_fine / 5120.0;
    return temperature;

}

BME280_U32_t calibration_P_int64(BME280_S32_t adc_P)
{

	BME280_S32_t var1;
	BME280_S32_t var2;
	BME280_S32_t var3;
	BME280_S32_t var4;
	BME280_U32_t var5;
	BME280_U32_t pressure;
	BME280_U32_t pressure_min = 30000;
	BME280_U32_t pressure_max = 110000;
    pressure = 0.0;

    var1 = (t_fine / 2.0) - 64000.0;
    var2 = (((var1 / 4.0) * (var1 / 4.0)) / 2048) * dig_P6;
    var2 = var2 + ((var1 * dig_P5) * 2.0);
    var2 = (var2 / 4.0) + (dig_P4 * 65536.0);
    var1 = (((dig_P3 * (((var1 / 4.0) * (var1 / 4.0)) / 8192)) / 8)  + ((dig_P2 * var1) / 2.0)) / 262144;
    var1 = ((32768 + var1) * dig_P1) / 32768;

    pressure = ((1048576 - adc_P) - (var2 / 4096)) * 3125;
    if (pressure < 0x80000000)
        pressure = (pressure * 2.0) / var1;
    else
        pressure = (pressure / var1) * 2;
    var1 = (dig_P9 * (((pressure / 8.0) * (pressure / 8.0)) / 8192.0)) / 4096;
    var2 = ((pressure / 4.0) * dig_P8) / 8192.0;
    pressure = pressure + ((var1 + var2 + dig_P7) / 16.0);
    return pressure/100;

}

BME280_S32_t calibration_H_int32(BME280_S32_t adc_H)
{
	BME280_S32_t v_x1_u32r;

    v_x1_u32r = t_fine - 76800.0;
    if (v_x1_u32r != 0)
    	v_x1_u32r = (adc_H - (dig_H4 * 64.0 + dig_H5/16384.0 * v_x1_u32r)) * (dig_H2 / 65536.0 * (1.0 + dig_H6 / 67108864.0 * v_x1_u32r * (1.0 + dig_H3 / 67108864.0 * v_x1_u32r)));
    v_x1_u32r = v_x1_u32r * (1.0 - dig_H1 * v_x1_u32r / 524288.0);
    if (v_x1_u32r > 100.0){
		v_x1_u32r = 100.0;
	}else if (v_x1_u32r < 0.0){
		v_x1_u32r = 0.0;
	}
    return v_x1_u32r;
}

void build_message_bme280(char* moji, int temperature, int pressure, int humidity) {
    char tempStr[12], pressStr[12], humStr[12];

    cf_itoa(temperature, tempStr);
    cf_itoa(pressure, pressStr);
    cf_itoa(humidity, humStr);

    cf_strcpy(moji, "T:");
    cf_strcat(moji, tempStr);
    cf_strcat(moji, ",P:");
    cf_strcat(moji, pressStr);
    cf_strcat(moji, ",H:");
    cf_strcat(moji, humStr);
    cf_strcat(moji, "\n");
}

// 急激な天候の変化をチェックする。10分前の気圧と現在の気圧を比較して、2hPa下がっている、かつ30C以上、かつ
// 湿度80％以上のとき、注意を促す。モデルとしての検証の必要あり
// return CLIMATE_CHK: 1, USUAL_STATE: 0
UB check_suddenly_climate_change()
{
	UB at_prev_idx = 0,at_ret = USUAL_STATE;
	// 気圧が2hPa以上下がっている
	if (curr_buf_idx_pos == 0){
		at_prev_idx = MAX_RECORD_NUM - 1;
	}else{
		at_prev_idx = curr_buf_idx_pos - 1;
	}
	if (((W)(pressure_every_min[curr_buf_idx_pos] - pressure_every_min[at_prev_idx]) < 2) &&
		(pressure_every_min[curr_buf_idx_pos] != 0) &&
		(pressure_every_min[at_prev_idx] != 0) &&
		(temperature_every_min[curr_buf_idx_pos] >= 30) &&
		(temperature_every_min[curr_buf_idx_pos] != 0) &&
		(humidity_every_min[curr_buf_idx_pos] >= 80) &&
		(humidity_every_min[curr_buf_idx_pos] != 0)
		){
		at_ret = CLIMATE_CHK;
	}else{
		at_ret = USUAL_STATE;
	}
	return at_ret;
}


// bme280タスク
void bme280_task(INT stacd, void *exinf)
{
	UB osrs_t = 1;
	UB osrs_p = 1;
	UB osrs_h = 1;
	UB mode   = 3;
	UB t_sb   = 5; //4
	UB filter = 0;
	UB spi3w_en = 0;
	UB ctrl_meas = (osrs_t << 5) | (osrs_p << 2) | mode;
	UB config    = (t_sb << 5) | (filter << 2) | spi3w_en;
	UB ctrl_hum  = osrs_h;
    B moji[21];
    static UB at_05sec_cnt = 0;

	write_reg(IICADR_BME,0xF2,ctrl_hum);
	write_reg(IICADR_BME,0xF4,ctrl_meas);
	write_reg(IICADR_BME,0xF5,config);
	ReadTrimmingParam();

	for (;;) {				// 約500msごとに永久に繰り返し
		readData();
		temperature = calibration_T_int32(temp_data);
		pressure    = calibration_P_int64(pres_data);
		humidity    = calibration_H_int32(hum_data);
		//temperature = temperature / 100;
		//pressure    = pressure / 25600;
		//pressure    = pressure / 100;
		//humidity    = humidity / 1024;
		tm_printf("T:%d,P:%d,H:%d\n",temperature,pressure,humidity);
		//sprintf(moji,"T:%d,P:%d,H:%d\n",temperature,pressure,humidity);
		build_message_bme280(moji,temperature,pressure,humidity);
		writeCommand(0x0+0x80);
		for(B i = 0;i < 20;i++){
			writeData(moji[i]);
		}
		// 120 = 0.5s x 60 = 1min
		if(++at_05sec_cnt >= 120){
			temperature_every_min[curr_buf_idx_pos] = temperature;
			pressure_every_min[curr_buf_idx_pos] = pressure;
			humidity_every_min[curr_buf_idx_pos] = humidity;
			if(check_suddenly_climate_change() == CLIMATE_CHK){
				cf_strcpy(moji, "CHECK!CLIMATE CHANGE!");
				writeCommand(0x0+0x80);
				for(B i = 0;i < 20;i++){
					writeData(moji[i]);
				}
			}
			at_05sec_cnt = 0;
			curr_buf_idx_pos = (++curr_buf_idx_pos>=MAX_RECORD_NUM)?0:curr_buf_idx_pos;
		}

		tk_dly_tsk(500);       // 約500ミリ秒の周期で永久に繰り返し
	}
}
