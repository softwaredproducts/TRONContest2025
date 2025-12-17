#include <tk/tkernel.h>
#include <tm/tmonitor.h>
#include "app_uart.h"

//-------- UARTEのレジスタ定義 ----------------------------------
#define	UARTE(p, r)	(UARTE##p##_BASE + UARTE_##r)

#define	UARTE0_BASE	0x40002000	// コンソール用UART0と競合するので使用しない
#define	UARTE1_BASE	0x40028000	// こちらを使用する

#define	UARTE_TASKS_STARTRX		(0x000)
#define	UARTE_TASKS_STOPRX		(0x004)
#define	UARTE_TASKS_STARTTX		(0x008)
#define	UARTE_TASKS_STOPTX		(0x00C)
#define	UARTE_TASKS_FLUSHRX		(0x02C)

#define	UARTE_EVENTS_CTS		(0x100)
#define	UARTE_EVENTS_NCTS		(0x104)
#define	UARTE_EVENTS_RXDRDY		(0x108)
#define	UARTE_EVENTS_ENDRX		(0x110)
#define	UARTE_EVENTS_TXDRDY		(0x11C)
#define	UARTE_EVENTS_ENDTX		(0x120)
#define	UARTE_EVENTS_ERROR		(0x124)
#define	UARTE_EVENTS_RXTO		(0x144)

#define	UARTE_EVENTS_RXSTARTED	(0x14C)
#define	UARTE_EVENTS_TXSTARTED	(0x150)
#define	UARTE_EVENTS_TXSTOPPED	(0x158)

#define	UARTE_SHORTS			(0x200)
#define	UARTE_INTEN				(0x300)
#define	UARTE_INTENSET			(0x304)
#define	UARTE_INTENCLR			(0x308)
#define	UARTE_ERRORSRC			(0x480)
#define	UARTE_ENABLE			(0x500)
#define	UARTE_PSEL_RTS			(0x508)
#define	UARTE_PSEL_TXD			(0x50C)
#define	UARTE_PSEL_CTS			(0x510)
#define	UARTE_PSEL_RXD			(0x514)
#define	UARTE_BAUDRATE			(0x524)

#define	UARTE_RXD_PTR			(0x534)
#define	UARTE_RXD_MAXCNT		(0x538)
#define	UARTE_RXD_AMOUNT		(0x53C)
#define	UARTE_TXD_PTR			(0x544)
#define	UARTE_TXD_MAXCNT		(0x548)
#define	UARTE_TXD_AMOUNT		(0x54C)

#define	UARTE_CONFIG			(0x56C)

//-------- ボードIDの取得 ---------------------------------------
#define	NRF5_FICR_BASE	0x10000000
#define	DEVICEID_0		0x060		// Device identifier 0
#define	DEVICEID_1		0x064		// Device identifier 1

// メインボード用micro:bitの実際のデバイスID
const W main_devid0 = 0xb5fcd839;	//
const W main_devid1 = 0xb1bd0a74;	//

W devid0, devid1;					// 実行中のmicro:bitのデバイスID

// 自分自身がメインボードの場合にTRUEを返す関数(※B)
BOOL is_main_board(){
	return(devid0 == main_devid0 && devid1 == main_devid1);
}

// ボードのデバイスIDを取得してコンソールに表示
void get_device_id(void)
{
	devid0 = in_w(NRF5_FICR_BASE + DEVICEID_0);	// DEVICE[0]の読み出し(※A)
	devid1 = in_w(NRF5_FICR_BASE + DEVICEID_1);	// DEVICE[1]の読み出し(※A)

	tm_printf("This devid_0_1: %08x_%08x\n", devid0, devid1);
	tm_printf("Main devid_0_1: %08x_%08x\n", main_devid0, main_devid1);
	if(is_main_board())
		tm_printf("This is MAIN board, P0TX_P1RX\n\n");
	else
		tm_printf("This is  SUB board, P1TX_P0RX\n\n");
}

//-------- ボタンスイッチに対応するGPIO P0のピンを入力に設定 ----
void btn_init(void)
{
	out_w(GPIO(P0, PIN_CNF(14)), 0);	// ボタンスイッチAはGPIO P0.14に接続
	out_w(GPIO(P0, PIN_CNF(23)), 0);	// ボタンスイッチBはGPIO P0.23に接続
}

// UARTEの送信データと受信データを入れるバッファメモリ
#define	MAXBUF	10
static _UB	txbuf[MAXBUF];
static _UB	rxbuf[MAXBUF];
static _UB  txbuf2[MAXBUF];
static _UB  rxbuf2[MAXBUF];

extern ID	flgid;					// 割込み通知用イベントフラグのID
extern ID	semid;					// 送信バッファ排他制御用セマフォのID

//-------- 割込み許可を含めたUARTE1の初期化とピン設定 -----------
//			最後の割込み許可以外は連載第13回のinit_uarte1と同じ
void init_uarte1(INT pin_txd, INT pin_rxd)
{
	out_w(UARTE(1, EVENTS_ENDRX), 0);
	out_w(UARTE(1, EVENTS_ENDTX), 0);
	out_w(UARTE(1, EVENTS_ERROR), 0);
	out_w(UARTE(1, ERRORSRC), 0);

	out_w(UARTE(1, BAUDRATE), 0x00275000);		// 9600 baud(※B)
	// out_w(UARTE(1, BAUDRATE), 0x01D7E000);	// 115200 baud(※B)

	// Hardware flow control無し、パリティ無し、One stop bit
	out_w(UARTE(1, CONFIG), 0);

	out_w(UARTE(1, PSEL_TXD), pin_txd);	// TXDのGPIOピン設定(※C)
	out_w(UARTE(1, PSEL_RXD), pin_rxd);	// RXDのGPIOピン設定(※C)
	out_w(UARTE(1, ENABLE), 8);			// Enable UARTE(※D)

	// EVENTS_ENDTXとEVENTS_ENDRXの割込み許可の設定
	out_w(UARTE(1, INTEN), 0x00000110);
}

//-------- UARTE1からtxdatの1バイトを送信開始(※C) --------------
//			ボタン状態変化により呼ばれる
void uarte1_start_tx1(UB *txdat){

	tk_wai_sem(semid, 1, TMO_FEVR);			// 送信バッファの排他制御(※D)

	//txbuf[0] = txdat;						// 送信データを送信バッファに格納
	memcpy(txbuf,txdat,MAXBUF);

	out_w(UARTE(1, TXD_PTR), (UW) txbuf);	// 送信バッファ先頭アドレス
	out_w(UARTE(1, TXD_MAXCNT), MAXBUF);			// 送信するバイト数

	out_w(UARTE(1, EVENTS_ENDTX), 0);		// 送信終了フラグのクリア
	out_w(UARTE(1, TASKS_STARTTX), 1);		// 送信開始
}

//-------- UARTE1で1バイトの受信開始(※E) -----------------------
//			起動後および受信完了の割込みハンドラから呼ばれる
void uarte1_start_rx1(){
	out_w(UARTE(1, RXD_PTR), (UW) rxbuf);	// 受信バッファ先頭アドレス
	out_w(UARTE(1, RXD_MAXCNT), MAXBUF);			// 受信するバイト数

	out_w(UARTE(1, EVENTS_ENDRX), 0);		// 受信終了フラグのクリア
	out_w(UARTE(1, TASKS_STARTRX), 1);		// 受信開始
}

//-------- UARTE1の送信完了と受信完了の割込みハンドラ -----------
//			送信完了と受信完了が同時に起こる可能性もあるので、
//			8ビットの送信データと受信データを1つの16ビットデータに
//			まとめてからset_flgで送る
void uarte1_inthdr(UINT intno)
{
	UW		txrxdat = 0;	// 受信データと送信データを入れてset_flgで送る変数
	_UB*	txrxadr;		// 送信バッファまたは受信バッファの先頭アドレス

	if(in_w(UARTE(1, EVENTS_ENDRX))){		// 受信完了の割込みの場合(※F)
		txrxadr = (_UB *) in_w(UARTE(1, RXD_PTR));
		txrxdat |= (UW) txrxadr[0];			// 下位8ビットに受信データを格納
		memcpy(rxbuf2,txrxadr,MAXBUF);
		uarte1_start_rx1();					// 次の受信の準備(※G)
	}

	if(in_w(UARTE(1, EVENTS_ENDTX))){		// 送信完了の割込みの場合(※H)
		txrxadr = (_UB *) in_w(UARTE(1, TXD_PTR));

		// txrxdatの上位8ビットに送信データを格納(※J)
		txrxdat |= ((UW) txrxadr[0] << 8);
		memcpy(txbuf2,txrxadr,MAXBUF);
		out_w(UARTE(1, EVENTS_ENDTX), 0);	// 送信終了フラグのクリア(※K)
		tk_sig_sem(semid, 1);				// 送信バッファの排他制御を解除(※L)
	}

	tk_set_flg(flgid, txrxdat);				// 受信データと送信データの通知(※M)
}

// 以前のボタンの状態を保持する変数
LOCAL BOOL prev_a = FALSE;
LOCAL BOOL prev_b = FALSE;

//---------------------------------------------------------------
// ボタンスイッチの状態をチェック(※P)
LOCAL void check2_btn(BOOL new_btn, BOOL *prev_btn, UB *btnchr){
	UB actchr;

	if(new_btn ==  *prev_btn) return;	// 状態変化が無ければ何もせずに戻る

	*prev_btn = new_btn;

	if(new_btn){						// ボタンが押された場合
		uarte1_start_tx1(btnchr);		// btnchrのデータをUARTE1で送信
	}
}

// ボタンスイッチの入力判定を行う送信側タスク(※N)
void btn_tx_task(INT stacd, void *exinf){
	UW gpio_p0in;
	BOOL btn_a, btn_b;



	for (;;) {				// 約100msごとに永久に繰り返し

		// GPIO P0のINレジスタを読んでgpio_p0inに設定
		gpio_p0in = in_w(GPIO(P0, IN));

		btn_a = ((gpio_p0in & (1 << 14)) == 0); // P0.14が0の場合にTRUE
		btn_b = ((gpio_p0in & (1 << 23)) == 0); // P0.23が0の場合にTRUE

		check2_btn(btn_a, &prev_a, "+REQPLACE+");	// ボタンAの変化のチェックと送信
		check2_btn(btn_b, &prev_b, "+DUMMYCND+");	// ボタンBの変化のチェックと送信

		tk_dly_tsk(100);					// 100msのディレイ
	}
}

// 表示できない文字を '.' に変換する関数
LOCAL UINT disp_ch(UINT ch){
	ch &= 0xff;
	if(ch < ' ' || ch > '~')	return('.');
	else						return(ch);
}

// イベントフラグで通知された値を表示する受信側タスク(※Q)
void flg_rx_task(INT stacd, void *exinf){
	UINT flgptn;
	UB	txchr, rxchr;

	for (;;) {				// 永久に繰り返し

		// 割込み通知用イベントフラグの下位2バイトをOR待ち
		tk_wai_flg(flgid, 0xffff, (TWF_ORW | TWF_CLR), &flgptn, TMO_FEVR);

		txchr = (flgptn >> 8) & 0xff;	// 送信データをtxchrに入れる(※R)
		rxchr = flgptn & 0xff;			// 受信データをrxchrに入れる

		tm_printf("flg_rx_task: dat=%04x, tx='%c', rx='%c'\n",
			(UINT) flgptn, disp_ch(txchr), disp_ch(rxchr));

		tm_printf("rcv:%c%c%c%c%c%c%c%c%c%c snd:%s\n", rxbuf2[0],rxbuf2[1],rxbuf2[2],rxbuf2[3],rxbuf2[4],rxbuf2[5],rxbuf2[6],rxbuf2[7],rxbuf2[8],rxbuf2[9], txbuf2);
		if(txchr == '+'){	// 送信の場合には、LEDに表示しない
			;
		}else{
			writeCommand(0x0+0x80);
			for(B i = 0;i < 10;i++){
				writeData(rxbuf2[i]);
			}
			for(B i = 0;i < 10;i++){
				writeData(' ');
			}
			tk_dly_tsk(200);
		}
	}
}
