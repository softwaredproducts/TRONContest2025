# 危険予知を支援するシステム

TRONプログラミングコンテスト2025のRTOSアプリケーション　一般部門に応募した「危険予知を支援するシステム」について、記述します

## 機能
以下の情報をキャッチして、OLEDに表示します
- 障害物との距離を測定
- 現在地情報を予測
- 異常気象を検知

## ハードウェア構成
危険予知を支援するシステムは、以下の部品で構成しています
- micro:bit V2
- Seeed Xiao ESP32C3 (サブ基板として使用)
- 超音波距離センサー(HC-SR04)
- OLED(SO1602AWGB-UC-WB-U)
- 温湿度・気圧センサー(AE-BME280)
- GPS (AE-GPS)
- USBハブ
- USBケーブル x2 (USB Type A- microB, USB Type A- TypeC)
- USBモバイル電源
- 3.3V電池x4と電池ホルダー
- ブレッドボード
- ジャンパワイヤー
- ポケットWifi

## ファイル・フォルダ構成
- operation_manual.pdf : 簡単な使い方の説明
- Kikenyochi4_sekiya_jp.pptx: [プレゼンに使った資料の更新版](Kikenyochi4_sekiya_jp.pptx)
- microbit        : micro:bitのuT-Kernel用プログラム
- xiao_esp32c3    : Seeed Xiao ESP32C3用プログラム
- LICENSE         : ライセンス
- README.md       : このファイル

## オペレーションマニュアル
- 基本的な使い方については、[operation_manual.pdf](operation_manual.pdf)をご参照ください

## microbitフォルダについて
以下は、microbit/mtkernel_3/app_sample内にあるファイル一覧です。

| ファイル名 | 処理内容 | 備考 | ライセンス |
|---------|---------|---------|---------|
| app_main.c | メインプログラム　起動時の処理、タスクの起動、OSリソースの確保 | | MIT |
| app_uart.c | UART送信、受信タスク | Personal Mediaのソースに対して、Aボタン押下時に、10バイトの現在地要求をサブ基板に送信し、サブ基板から10バイトのデータを受信するように変更　受信結果をOLEDに表示 | Personal Media Corporation |
| app_uart.h | UART送信、受信タスク | | Personal Media Corporation |
| bme280.c | 温湿度・気圧センサータスク | BME280センサー使用 | MIT |
| bme280.h | 温湿度・気圧センサータスク | | MIT |
| common_func.c | 共通関数 | | MIT |
| common_func.h | 共通関数 | | MIT |
| gpiote.h | GPIOTEレジスター定義 | | Personal Media Corporation |
| iic_reg.c | I2C関連 | Personal Mediaのソースに対して、read_reg_bme280関数を追加 気象データ取得時のエラー発生時のため、エラーコードとデータを分離| Personal Media Corporation |
| iic.h | I2C関連 | | Personal Media Corporation |
| nrf5_iic.h | I2C関連 | | Personal Media Corporation |
| oled.c | OLED制御関数 | | MIT |
| oled.h | OLED制御関数 | | MIT |
| ultrasonic.c | 超音波タスク | | MIT |
| ultrasonic.h | 超音波タスク | | MIT |

### 必要環境
- micro:bit V2
- Eclipse IDE
- 環境構築等は、Personal Media Corporation様のIoTエッジノード実践キット micro:bit版 Rel 1.01 CD-ROMを参考にさせていただきました
- 環境構築後のapp_sampleフォルダに、上記のmicrobit/mtkernel_3/app_sample内の全ファイルで上書きします

※I2C関連は下記を参考にさせていただきました
- [［第12回］I2C経由で加速度センサーを使ってみよう](https://www.t-engine4u.com/info/mbit/12.html)
- [［第13回］2台のmicro:bitをシリアル通信で接続](https://www.t-engine4u.com/info/mbit/13.html)

## xiao_esp32c3フォルダについて
xiao_esp32c3/tronprj内にプログラムがあります

| ファイル名 | 処理内容 | 備考 | ライセンス |
|---------|---------|---------|---------|
| tronprj.ino | 現在地要求をmicro:bitから受信時に、GPS情報から、chatgptが現在地予測を行い、結果を10バイトずつに分割して、micro:bitに10バイトずつ返信 | | MIT |


### 必要環境
- Seeed Xiao ESP32C3
- Arduino IDE
- ライブラリ:
  - [TinyGPS++](https://github.com/mikalhart/TinyGPSPlus) 
  - WiFi.h(ESP32標準)
  - HTTPClient(ESP32標準)

※Seeed Xiao ESP32C3については、以下を参考にさせていただきました
 - [Getting Started with Seeed Studio XIAO ESP32C3](https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/)
 - [Learn to use WiFiClient and HTTPClient on XIAO ESP32C3 - XIAO ESP32C3 & ChatGPT in action](https://wiki.seeedstudio.com/xiaoesp32c3-chatgpt/)

### 環境設定
 以下の箇所では、WifiのSSID名とパスワード、および、ChatgptのAPIキーを記述します。ご自身の環境に合わせて、変更する必要があります
 ```
 // Replace with your network credentials
 const char* ssid = "";
 const char* password = "";

 //chatgpt api Replace with your token of ChatGPT
 const char* chatgpt_token = "";
 ```

