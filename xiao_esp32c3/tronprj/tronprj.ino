#include <HardwareSerial.h>
#include <TinyGPS++.h>
// Load Wi-Fi library
#include "WiFi.h"
#include <HTTPClient.h>

// Replace with your network credentials
const char* ssid = "";
const char* password = "";

//chatgpt api Replace with your token of ChatGPT
const char* chatgpt_token = "";
char chatgpt_server[] = "https://api.openai.com/v1/completions";

// Set web server port number to 80
//WiFiServer server(80);
//WiFiClient client1;

HTTPClient https;
String chatgpt_Q;
String chatgpt_A;
#define MAX_CHUNKS 50
String chunkStr[MAX_CHUNKS];
int chunknum = 0;
int httpCode = 0;
uint16_t dataStart = 0;
uint16_t dataEnd = 0;
String dataStr;
uint8_t send_data_idx = 0;


typedef enum 
{
  init_state,
  send_chatgpt_request,
  get_chatgpt_list,
  send_data,
}STATE_;

STATE_ currentState;

// gps
String command = "";
TinyGPSPlus gps;
HardwareSerial gpsSerial(1);

// serial
uint8_t buffer[10];
int idx = 0;

void WiFiConnect(void){
    int retry_counter = 0;
    WiFi.begin(ssid, password);
    //Serial.print("Connecting to ");
    //Serial.println(ssid);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        //Serial.print(".");
        if(++retry_counter >15){
          break;
        }
    }
    //Serial.println("");
    //Serial.println("WiFi connected!");
    //Serial.print("IP address: ");
    //Serial.println(WiFi.localIP());
    currentState = init_state;
    //currentState = send_chatgpt_request;
}

// setup
void setup() {
    // Set WiFi to station mode and disconnect from an AP if it was previously connected
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();    
    WiFiConnect();

    Serial.begin(9600);
    gpsSerial.begin(9600,SERIAL_8N1,9,10);
    while (!Serial);

    //Serial.println("WiFi Setup done!");

}
 
// loop
void loop() {
    gps.encode(gpsSerial.read());
    /*
    sprintf((char*)buffer,"L%8.5f",gps.location.lat());
    Serial.write(buffer,10);
    Serial.println();
    delay(300);
    sprintf((char*)buffer,"G%8.5f",gps.location.lng());
    Serial.write(buffer,10);
    Serial.println();
    delay(300);
    */
/*
    if (gps.location.isUpdated() && Serial.available()) {
        //sprintf(buffer,"L%009.6f",gps.location.lat());
        sprintf((char*)buffer,"L%8.5f",gps.location.lat());
        Serial.write(buffer,10);
        delay(300);
        sprintf((char*)buffer,"G%8.5f",gps.location.lng());
        Serial.write(buffer,10);
        delay(300);
    }
    */
  switch(currentState){
    case init_state:
      break;
    case send_chatgpt_request:
      //Serial.println("send_chatgpt_req");
      // internet is enable and gps information has effective value
      if (https.begin(chatgpt_server)) {  // HTTPS
        //Serial.println("http begin loop");
        gps.encode(gpsSerial.read());
        if(gps.location.lat() >= 1.0 && gps.location.lng() >= 1.0){
          //Serial.print("1. step1");
          chatgpt_Q = "where is " + String(gps.location.lat()) + ", " + String(gps.location.lng()) + "in google map?";
          https.addHeader("Content-Type", "application/json"); 
          String token_key = String("Bearer ") + chatgpt_token;
          https.addHeader("Authorization", token_key);
          String payload = String("{\"model\": \"gpt-3.5-turbo-instruct\", \"prompt\": \"") + chatgpt_Q + String("\", \"temperature\": 0, \"max_tokens\": 100}"); //Instead of TEXT as Payload, can be JSON as Paylaod
          //String payload = String("{\"model\": \"gpt-4\", \"prompt\": \"") + chatgpt_Q + String("\", \"temperature\": 0, \"max_tokens\": 100}"); //Instead of TEXT as Payload, can be JSON as Paylaod
          httpCode = https.POST(payload);   // start connection and send HTTP header
          payload = "";
          //currentState = get_chatgpt_list;
        }else{
          ;
        }
      }
      else {
        ;
      }
      currentState = get_chatgpt_list;
      delay(1000);
      break;
    case get_chatgpt_list:
      if(gps.location.lat() < 1.0 || gps.location.lng() < 1.0){
          chunknum = 1;
          chunkStr[0] = "Check GPS ";
          //currentState = send_data;
      }else if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = https.getString();
        dataStart = payload.indexOf("\\n\\n") + strlen("\\n\\n");
        dataEnd = payload.indexOf("\",", dataStart); 
        chatgpt_A = payload.substring(dataStart, dataEnd);
        //Serial.println("2. step" );
        //Serial.println(chatgpt_A); // pass here
        int chunkSize = 10;
        chunknum = (chatgpt_A.length() + 5) /chunkSize ;
        //Serial.print(chatgpt_A);
        //Serial.print("chunknum->");
        //Serial.print(chunknum);
        for (int i = 0; i < chatgpt_A.length(); i += chunkSize) {
          static int j = 0;
          chunkStr[j] = chatgpt_A.substring(i, i + chunkSize);
          j++;
          //Serial.println(chunkStr[i]);
        }
        //currentState = send_data;
        https.end();
      }else {
          chunknum = 1;
          chunkStr[0] = "com error ";
          //currentState = send_data;
      }
      currentState = send_data;
      delay(1000);
      break;
    case send_data:
      // if the each string length is less than 10 bytes, fill by space
      if(chunkStr[send_data_idx].length() < 10){
        while (chunkStr[send_data_idx].length() < 10) {
          chunkStr[send_data_idx] += ' '; 
        }
      }else{
        ;
      }
      // send fixed 10 bytes
      Serial.write(chunkStr[send_data_idx].c_str(),10);
      //Serial.println(chunkStr[send_data_idx]);
      if(++send_data_idx >= chunknum || send_data_idx > MAX_CHUNKS){
        //Serial.println("reset logic");
        send_data_idx = 0;
        currentState = init_state;
      }
      delay(500);
      break;
  }
}

bool responded = false;
int lastSent = -1;  // 前回送信した値（初期値は無効）

// シリアル受信時に呼ばれる
void serialEvent() {
  while (Serial.available()) {
    int val = Serial.read();
    buffer[idx++] = (uint8_t)val;
    if (idx == 10) {
      // if the command is from button A
      if( memcmp(buffer,"+REQPLACE+",10) == 0 &&
          currentState == init_state
      ){
        currentState = send_chatgpt_request;
      }else{
        ;
      }
      idx = 0;  // バッファをリセット
    }
  }
}


