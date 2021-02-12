/*
 * Copyright (c) 2017, Washow Corppration
 * All rights reserved.
 *
 * This code is only designed for ARDUINO NANO and SIC9311/SIC4310.
 * All timings and sequences were suitable for in specified application
 * And only use for programming reference.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <Arduino.h>
#include "FlashStorage.h"
#include <U8g2lib.h>
#ifdef U8X8_HAVE_HW_I2C
  #include <Wire.h>
#endif
#include "SIC9310_SPI.h"
#include "SIC9310.h"
#include "T2tag.h"
#include <SPI.h>
#define DEBUG
#define VERSION "TagEncoder ver1.f"

#define CS 10
#define BTN_SEL   9
#define LED_ERROR 7
#define LED_BUSY  6
#define LED_READY 5
#define LED_DONE  4
#define BUZZER    3
#define BTN_OK    2

#define MODE 5
#define TAG_PAGE_SIZE 512
#define NDEF_MESSAGE_NUMBER 4
SIC9310_SPI sic9310spi(SPI, CS);
SIC9310 nfc = SIC9310(sic9310spi);
T2tag tag = T2tag(nfc);

//byte rxbuf[TAG_MEMORY_SIZE];
int mode_state = LOW;
byte currOpMode;
byte currTxPWR;
byte currProjType;
byte numPGM;
int ic_type = 0;
int tag_in = 0;
uint8_t mode = 0;
long mode_time, tag_time, setting_time;
uint8_t indexMsg = 0;
int ledMap[4]={LED_DONE, LED_READY, LED_BUSY, LED_ERROR};
byte ledStatus=0;
//byte prevUID[7];
byte nowUID[7];
bool uid_compare;
int pubCNT = 0;
//+++++++++++++++++++++++++++++
typedef struct {
  boolean activate;
  int reqPGM;
  int numPGM;
  byte ndef_mem[450];
} iProj;
FlashStorage(PP0_EVM, iProj);
iProj ppFiles;


U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
//------OLED display
#define OK  0
#define OK_IN  1
#define NG_OUT 2
#define TMP 3
#define NG_ERR 4
#define NONE 5
#define GUARD 6
#define POS 7
#define LED_TIME 350

String ss = "";
String st = "";
String sl = "";

void drawTPSSymbol(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol)
{
  // fonts used:
  // u8g2_font_open_iconic_check_6x_t
  // u8g2_font_open_iconic_mime_6x_t
  // encoding values, see: https://github.com/olikraus/u8g2/wiki/fntgrpiconic
  
  switch(symbol)
  {
    case OK:
      u8g2.setFont(u8g2_font_open_iconic_check_4x_t);
      u8g2.drawGlyph(x, y, 64); 
      break;
    case OK_IN:
      u8g2.setFont(u8g2_font_open_iconic_check_4x_t);
      u8g2.drawGlyph(x, y, 65); 
      break;
    case NG_OUT:
      u8g2.setFont(u8g2_font_open_iconic_check_4x_t);
      u8g2.drawGlyph(x, y, 66); 
      break;
    case TMP:
      u8g2.setFont(u8g2_font_open_iconic_check_4x_t);
      u8g2.drawGlyph(x, y, 67); 
      break;
    case NG_ERR:
      u8g2.setFont(u8g2_font_open_iconic_check_4x_t);
      u8g2.drawGlyph(x, y, 68);
      break; 
    case NONE:
      u8g2.setFont(u8g2_font_open_iconic_app_4x_t);
      u8g2.drawGlyph(x, y, 64);
      break;
    case GUARD:
      u8g2.setFont(u8g2_font_open_iconic_app_4x_t);
      u8g2.drawGlyph(x, y, 71);
      break;     
    case POS:
      u8g2.setFont(u8g2_font_open_iconic_app_4x_t);
      u8g2.drawGlyph(x, y, 65);
      break;        
  }
}

void drawTPS(uint8_t symbol, byte kind)
{
  drawTPSSymbol(0, 48, symbol);
  u8g2.setFont(u8g2_font_logisoso32_tr);
  u8g2.setCursor(32+3, 48);
  switch (kind) {
    case 0x38:
      u8g2.print("<-O");
      break;
    case 0xF5:
      char tt[4];
      sprintf(tt, "%04d", pubCNT); 
      u8g2.print(String(tt));
      break;
    case 0xF6:
      u8g2.print("PUB");
      break;
    case 0xFA:
      u8g2.print("SUCC");
      break;
    case 0xFB:
      u8g2.print("PROT");
      break;
    case 0xFC:
      u8g2.print("D:NG");
      break;
    case 0xFD:
      u8g2.print("INIT");
      break;
    case 0xFE:
      u8g2.print("ERR");
      break;
    case 0xFF:
      u8g2.print(sl);
      break;
    default:
      u8g2.print("CHEK");
      break;
  }
}

void draw(uint8_t symbol, byte kind)
{
    u8g2.firstPage();
    do {
      drawTPS(symbol, kind);
      u8g2.setFont(u8g2_font_ncenB08_tr);
      u8g2.setCursor(0, 12);
      u8g2.print(st); //"TopPASS Initialor V1"
      u8g2.setCursor(0, 62);
      u8g2.print(ss);
      //drawScrollString(offset, s);
    } while ( u8g2.nextPage() );
    delay(20);
}

void setup(){
  Serial.begin(115200);
  Serial.println(VERSION);
  delay(100);
  pinMode(LED_ERROR, OUTPUT);
  pinMode(LED_BUSY, OUTPUT);
  pinMode(LED_READY, OUTPUT);
  pinMode(LED_DONE, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BTN_SEL, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);
  ppFiles = PP0_EVM.read();
  u8g2.begin();  
  u8g2.enableUTF8Print();
  ledStatus = 0b00011111;
  showLED();
  delay(1000);
  ledStatus=0b00000000;
  showLED();
  nfc.begin();
  int rr = 0;
  do {
      rr = nfc.checkDEVICE();
  } while ( rr == -1);
  delay(500);
  nfc.config14443A();
  alarmBeep(BUZZER,100);
  ledStatus=0b00000010;
  showLED();
  delay(50);
  st = VERSION;
  ss = "PLS Setup first";
  draw(NONE, 0x39);
  delay(500);
}

void loop(){
  switch(mode)
  {  
      case 0: //setup mode: Prepare NDEF
        st = VERSION;
        ss = "[SEL] to Start..";
        draw(GUARD, 0xFD);
        if ((digitalRead(BTN_SEL) == LOW) & (!tag_in)) {
              delay(200);
              if (digitalRead(BTN_SEL) == LOW) {
                    alarmBeep(BUZZER, 200); 
                    delay(200);
                    go_menu();
              }
        }
        
        if ((digitalRead(BTN_OK) == LOW) & (!tag_in)) {
              delay(200);
              if (digitalRead(BTN_OK) == LOW) {
                   mode = 1;
                   st = VERSION;
                   ss = "[OK] to Setup Menu..";
                   draw(OK_IN, 0xF6);
                   alarmBeep(BUZZER, 200); 
                   delay(200);
              }
        }
        break;

      case 1: //publish mode
        int page=0;
        byte rx[4],tx[4];
        if ((digitalRead(BTN_OK) == LOW) & (!tag_in)) {
              delay(200);
              if (digitalRead(BTN_OK) == LOW) {
                   mode = 0;
                   alarmBeep(BUZZER, 200); 
                   delay(200);
                   break;
              }
        }
        nfc.idle();
        uint8_t rr = nfc.isTagPresent(nowUID);
        if(rr < 9) { // one tag in the field..
              ledStatus = 0b00000000;
              tag_in=1;
              //Serial.print("uid_compare=");Serial.println(rr, DEC);
              switch (rr) { 
                  case 0: //same tag was still in the field..
                    ledStatus = 0b00000100;
                    ss = "Pls Move to Next..";
                    draw(NONE, 0x38);
                    break;
                  case 1: //new tag in the field..
                    st = "";
                    for(int i =0; i < 7; i++){
                        char tt[2] ={0};
                        sprintf(tt, "%02X ", nowUID[i]); 
                        st = st + String(tt);
                    }
                    Serial.println(st);
                    ss = "Pls Hold Tag..";
                    draw(OK_IN, 0xF6);
                    ledStatus = 0b00000001;
                    showLED();
                    delay(100);
                    // Write URL to tag..
                    ss = "Now writing..";
                    draw(OK_IN, 0xF6);
                    delay(300);
                    // Done and Read to verify
                    pubCNT ++;
                    delay(300);
                    st = "";
                    ss = "Done and Next..";
                    draw(OK, 0xF5);
                    ledStatus = 0b00000000;
                    showLED();
                    //
                    alarmBeep(BUZZER,100);
                    delay(100);
                    break;
                case 2: //error: previous tag in the field..
                   ledStatus = 0b00001000;
                   ss = "Err: PrevTag..";
                   draw(NONE, 0x38);
                   break;
              }
              //ledStatus |= 0b00000010;
              showLED();
              delay(200);
              /*
              //--------Read Page Testing;      
              if(nfc.T2tag_ReadPage(16, rx)==0){
                  ledStatus |= 0b00000000;
              }else{
                  ledStatus |= 0b00000100;        
              }
               
              //--------write Page Testing;
              for(int i=0; i < 4; i++){
                  tx[i]= random(0, 256);
              }    
              if(nfc.T2tag_WritePage(16,tx)==1){
                  ledStatus |= 0b00001000;
              }else{
                  ledStatus |= 0b00000000;
              }
              showLED();
              alarmBeep(BUZZER);
              delay(300);*/
        } else { //no tag in the field..
              tag_in=0;
              st = VERSION;
              ss = "Put tag to publish.."; 
              draw(NONE, 0x38);
              delay(300);
              ledStatus=0b00000000;
              showLED();
              delay(300);   
        }
        ledStatus=0b00000010;
        showLED();
        delay(300);
        break;
  }
}

void alarmBeep(int pin, int tme) {
  tone(pin, 5800, tme);
  //delay(2000); 
}
  
void showLED() {
  for(int i =0; i< 4; i++){
    digitalWrite(ledMap[i], (ledStatus&(1<<i))!=0?HIGH:LOW);
  }
}

void go_menu() {
  //-----[MENU#1]---------
   setting_time = millis();
   st = "1/4-Set OpMode-";
   ss = "Press to Select..";
   do {
      switch (currOpMode) {
          case 0x00: sl = "AUTO"; break; 
          case 0x01: sl = "STEP"; break;
      }
      draw(POS, 0xFF);
      delay(200);
      if (digitalRead(BTN_SEL) == LOW) {
            if (currOpMode == 0x01) {
                currOpMode = 0;
            } else {
                currOpMode ++;
            }  
            alarmBeep(BUZZER, 50);
            setting_time = millis();
      }
   } while ( millis() - setting_time < 2000 ); 
   alarmBeep(BUZZER, 200); 
   delay(200);
//-----[MENU#2]---------
   setting_time = millis();
   st = "2/4- Set TxPwr.";
   ss = "3F(max) .. 03(min) ";
   currTxPWR = nfc.setTxPwr(0x00);
   do {
      char tt[2] ={0};
      sprintf(tt, "%02X", currTxPWR);
      sl = "Tx:" + String(tt);
      draw(POS, 0xFF);
      delay(200);
      if (digitalRead(BTN_SEL) == LOW) {
            if (currTxPWR >= 0x3F) {
                currTxPWR = 0x03;
            } else {
                currTxPWR = currTxPWR + 0x0F;
            }  
            alarmBeep(BUZZER, 50);
            setting_time = millis();
      }
   } while ( millis() - setting_time < 2000 );
   currTxPWR = nfc.setTxPwr(currTxPWR); //cw conductance
   alarmBeep(BUZZER, 200); 
   delay(200);
//-----[MENU#3]---------
   setting_time = millis();
   st = "3/4-TotalN of Tag.";
   ss = "1000(max), 0 for Test.";
   do {
      char tt[2] ={0};
      sprintf(tt, "%02d", numPGM);
      sl = String(tt) + "00";
      draw(POS, 0xFF);
      delay(200);
      if (digitalRead(BTN_SEL) == LOW) {
            if (numPGM == 0) {
                numPGM = 10;
             } else {
                numPGM --;
             }  
            alarmBeep(BUZZER, 50); 
            setting_time = millis();
      }
   } while ( millis() - setting_time < 2000 );
   alarmBeep(BUZZER, 200); 
   delay(200);
//-----[MENU#4]---------
   setting_time = millis();
   st = "4/4-Load PROJ & NDEF";
   ss = "Max = 450bytes ";
   int totalMems = 0;
   do {
      char tt[2] ={0};
      sprintf(tt, "%02d", currProjType);
      sl = "PT:" + String(tt);
      draw(POS, 0xFF);
      delay(200);
      if (digitalRead(BTN_SEL) == LOW) {
            if (currProjType == 3) {
                currProjType = 0;
            } else {
                currProjType = currProjType + 1;
            }  
            alarmBeep(BUZZER, 50);
            setting_time = millis();
      }
   } while ( millis() - setting_time < 2000 );
   alarmBeep(BUZZER, 200); 
   delay(200);
   if (currProjType == 1) {
      bool rr = loadNDEF();
   }
//-----[MENU_END]---------   
}

bool loadNDEF() {
  int page = 4;
  byte rx[4];
  int result;
  nfc.idle();
  if( nfc.isTagPresent(nowUID) < 9 ){
     // read the data
     for (page = 4; page < TAG_PAGE_SIZE; page++) {
          result = nfc.T2tag_ReadPage(page, rx);
  }
  if (!result){
        Serial.print("Read failed! @ Page=0x");
        Serial.print(page,HEX); Serial.println(" "); 
        // TODO error handling
        return false;
  }
  Serial.print("Page:0x");
  Serial.print(page, HEX); 
  char dataString[20] = {0};
  sprintf(dataString, "%02X = %02X:%02X:%02X:%02X",page,rx[0],rx[1],rx[2],rx[3]);
  Serial.print(dataString);
  Serial.println(" "); 
  }
  return true;
}

