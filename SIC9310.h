#ifndef __SIC9310_H__
#define __SIC9310_H__
#include "SIC9310_SPI.h"
#include <Arduino.h>;
class SIC9310{
  public:
    static uint8_t PLUS0DB;
    static uint8_t PLUS3DB;
    static uint8_t PLUS6DB;
    static uint8_t PLUS9DB;
    static uint8_t PLUS12DB;
    static uint8_t PLUS15DB;
    static uint8_t PLUS18DB;
    static uint8_t PLUS21DB;
    SIC9310(SIC9310_SPI& interface);
    void begin();
    int checkSPI();
    int checkDEVICE();
    void powerDown();
    void powerOn();
    void config14443A();
    void rfOn(uint8_t  config_type);    
    void idle();
    void congifGain(byte power); 
    void clearError();
    uint16_t reqa();
    int antiColl14443A(uint8_t type);
    uint8_t select14443A(uint8_t type);
    uint8_t T2tag_ReadPage(int page, byte* data);
    uint8_t T2tag_WritePage(int page, byte* data);
    uint8_t isTagPresent(byte * nowUID); // tagAvailable
    byte setTxPwr(byte data);
    byte prevUID[7];
    //byte nowUID[7];
    bool uid_compare;
    
  private:
    SIC9310_SPI* _interface;
    byte uid_temp[15];
    byte uid[7];
    byte mUidLength;
    uint8_t uidLength;
    int getTagId(byte* uid);    
  
};


#endif
