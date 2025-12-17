/*---------------------------------------------------------------
 *	KIKEN YOCHI SYS
 *	Softwaredproducts
 *---------------------------------------------------------------*/
#include <tk/tkernel.h>
#include <tm/tmonitor.h>
#include "iic.h"			// I2C関連ファイルのインクルード(※A)
#include "app_uart.h"
#include "ultrasonic.h"
#include "bme280.h"

//-------- iic_reg.cで定義された関数のインポート ----------------
IMPORT INT read_reg( INT adr, INT reg );
IMPORT ER write_reg( INT adr, INT reg, UB dat );
IMPORT ER read_reg_6( INT adr, INT reg, UB dat[6] );
IMPORT ER write_reg_2(INT adr, UB dat[2]);
IMPORT ER read_reg_4(INT adr,  UB dat[2]);
IMPORT ER BytesRead(INT adr, INT reg, UB* dat, UB length);
IMPORT INT read_reg_bme280(INT adr, INT reg, UB* rv_dat);

//---------------------------------------------------------------
// タスクのオブジェクトID番号
ID uarttxtskid;				// UART送信側タスク
ID uartrxtskid;				// UART受信側タスク
ID ultrasonictskid;            // Ultrasonicタスク
ID oledtskid;                  // OLEDタスク
ID bme280tskid;                // BME280タスク

// タスク、イベントフラグ、セマフォの生成情報
const T_CTSK cuarttxtsk     = {0, (TA_HLNG | TA_RNG3), &btn_tx_task,     10, 1024, 0};
const T_CTSK cuartrxtsk     = {0, (TA_HLNG | TA_RNG3), &flg_rx_task,     10, 1024, 0};
const T_CTSK cultrasonictsk = {0, (TA_HLNG | TA_RNG3), &ultrasonic_task,  4, 1024, 0};
//const T_CTSK coledtsk       = {0, (TA_HLNG | TA_RNG3), &oled_task,       12, 1024, 0};
const T_CTSK cbme280tsk     = {0, (TA_HLNG | TA_RNG3), &bme280_task,     13, 1024, 0};
const T_CFLG cflg           = {0, (TA_TFIFO | TA_WSGL), 0};
const T_CSEM csem           = {0, (TA_TFIFO | TA_FIRST ), 1, 1};
const T_CSEM coledsem       = {0, (TA_TFIFO | TA_FIRST ), 1, 1};

ID	flgid;					// 割込み通知用イベントフラグのID
ID	semid;					// 送信バッファ排他制御用セマフォのID
ID  oledsemid;              // OLED 制御用セマフォID

// 割込みハンドラ定義情報
const T_DINT dint_uarte = {TA_HLNG, uarte1_inthdr};

// UARTE1の割込み番号と割込み優先度レベル
#define INTNO_UARTE1	((UARTE1_BASE >> 12) & 0x3f)	// 割込み番号
#define INTPRI_UARTE1	6								// 割込み優先度レベル

// メインプログラム
EXPORT void usermain( void ){
	INT tx_pin, rx_pin;			// UARTEの送受信用エッジコネクタ端子の定義

	get_device_id();
	if(is_main_board()){		// メインボードとサブボードの判定(※S)
		tx_pin = 2; rx_pin = 3;	// メインボードの場合はP0TX_P1RX相当
	} else {
		tx_pin = 3; rx_pin = 2;	// メインボード以外の場合はP1TX_P0RX相当
	}

	btn_init();					// ボタンスイッチ入力のためのGPIOの初期設定

	init_uarte1(tx_pin, rx_pin);		// UARTE1の初期化と送受信用端子の指定
	uarte1_start_rx1();					// 最初の受信の準備

	flgid = tk_cre_flg(&cflg);			// 割込み通知用のイベントフラグ生成
	semid = tk_cre_sem(&csem);			// 送信バッファ排他制御用のセマフォ生成
	//oledsemid = tk_cre_sem(&coledsem);

	iicsetup(TRUE);								// I2Cドライバ起動(※B)

	// OLED
	init_oled();

	B moji[] = "KIKEN YOCHI SYSTEM";
	for(B i = 0;i < 20;i++){
		writeData(moji[i]);
	}
	writeCommand(0x20+0x80);
	memcpy(moji, "SoftwaredProducts",20);
	for(B i = 0;i < 20;i++){
		writeData(moji[i]);
	}
	contrast_max();
	tk_dly_tsk(2000);

	uarttxtskid     = tk_cre_tsk(&cuarttxtsk);		// UART送信側タスクの生成
	uartrxtskid     = tk_cre_tsk(&cuartrxtsk);		// UART受信側タスクの生成
	ultrasonictskid = tk_cre_tsk(&cultrasonictsk);	// ultrasonicタスクの生成
//	oledtskid       = tk_cre_tsk(&coledtsk);		// OLEDタスクの生成
	bme280tskid     = tk_cre_tsk(&cbme280tsk);		// BME280タスクの生成
	tk_sta_tsk(uarttxtskid, 0);				// UART送信側タスクの起動
	tk_sta_tsk(uartrxtskid, 0);				// UART受信側タスクの起動
	tk_sta_tsk(ultrasonictskid,0);          // ultrasonicタスクの起動
	tk_sta_tsk(bme280tskid,0);              // BME280タスクの起動

	tk_def_int(INTNO_UARTE1, &dint_uarte);	// UARTEの割込みハンドラ定義
	EnableInt(INTNO_UARTE1, INTPRI_UARTE1);	// 割込み許可

	tk_slp_tsk(TMO_FEVR);				// 永久待ち、以下は実行しない
}
