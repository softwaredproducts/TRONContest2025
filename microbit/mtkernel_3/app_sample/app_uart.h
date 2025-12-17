#ifndef _APP_UART_H
#define _APP_UART_H

#include <tk/typedef.h>

#define	UARTE0_BASE	0x40002000	// コンソール用UART0と競合するので使用しない
#define	UARTE1_BASE	0x40028000	// こちらを使用する

extern ID	flgid;					// 割込み通知用イベントフラグのID
extern ID	semid;					// 送信バッファ排他制御用セマフォのID

void btn_tx_task(INT stacd, void *exinf);
void flg_rx_task(INT stacd, void *exinf);
void uarte1_inthdr(UINT intno);
void get_device_id(void);
BOOL is_main_board();
void btn_init(void);
void init_uarte1(INT pin_txd, INT pin_rxd);
void uarte1_start_tx1(UB *txdat);
void uarte1_start_rx1();

#endif //_APP_UART_H
