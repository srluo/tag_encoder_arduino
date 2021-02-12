#include "SIC9310.h"
//#define DEBUG
#define HAL(func)   (_interface->func)
SIC9310::SIC9310(SIC9310_SPI& interface){
  _interface = &interface;
}
uint8_t SIC9310::PLUS0DB = 0;
uint8_t SIC9310::PLUS3DB = 1;
uint8_t SIC9310::PLUS6DB = 2;
uint8_t SIC9310::PLUS9DB = 3;
uint8_t SIC9310::PLUS12DB = 4;
uint8_t SIC9310::PLUS15DB = 5;
uint8_t SIC9310::PLUS18DB = 6;
uint8_t SIC9310::PLUS21DB = 7;
void SIC9310::begin(){
  HAL(begin)();
  
}
void SIC9310::congifGain(byte power){
  byte buf;
  power <<= 3;
  HAL(writeReg)(GAIN_ST3, power);
}
int SIC9310::checkSPI(){
  byte buf = HAL(readReg)(PRIMARY_STATUS_REG);
  if(buf == 0x05){
    return 0; //ERR found
  }else if(buf == 0){
    return -1; //OK
  }
  else{
    return buf;
  }
  return buf;
}
int SIC9310::checkDEVICE(){
  byte buf = HAL(readReg)(DEVICE_TYPE_REG);
  if((buf == 0x7F) || (buf == 0xBF)){
    return 410; //SIC9410
  }else if(buf == 0x3B){
    return 311; //SIC9311
  }else if(buf == 0xA1){
    return 101; //SIC9101
  }else if(buf == 0x00){
    HAL(writeReg)(DECODER_CONTROL_REG, 0x70);
    byte buf2 = HAL(readReg)(DECODER_CONTROL_REG);
    if(buf2 == 0x68){
      return 100; //SIC9100
    }else if(buf2 == 0xB9){
      return 310; //SIC9310
    }else{
      return -1; //unknown
    }
  }else{
  return 0; //unknown  
  }
}
void SIC9310::powerDown(){
  byte buf = HAL(readReg)(CONTROL_REG);
  buf |= 0b00010000;
  HAL(writeReg)(CONTROL_REG, buf);
}
void SIC9310::powerOn(){
  byte buf = HAL(readReg)(CONTROL_REG);
  buf &= 0b11101111;
  HAL(writeReg)(CONTROL_REG, buf);
}
void SIC9310::rfOn(uint8_t  config_type ){ 
  byte buf = HAL(readReg)(TX_CONTROL_REG);  
  // Set Only Force100ASK Bit (Bit4)  
  buf |= 0x10;
  if ( config_type == DRIVER_CONFIG_X_E50OUT ){ // For Class-E Antenna
    // Read Register 0x11 before write
    buf = (buf & Force100ASK_Clear_Mask 
      & TX2Inv_Clear_Mask 
      & TX1RFEn_Clear_Mask)
      | TX2Cw_Set_Mask 
      | TX2RFEn_Set_Mask;
    HAL(writeReg)(TX_CONTROL_REG, buf);  
  }else{
    // Set Only Bit TX2RFEn and TX1RFEn (Bit1:0)    
    buf = buf | TX2RFEn_Set_Mask 
      | TX1RFEn_Set_Mask;    
    HAL(writeReg)(TX_CONTROL_REG, buf);  
  }
}

//config sic9310 for 14443A protocol
void SIC9310::config14443A(){
  
  HAL(writeReg)(TX_CONTROL_REG, 0x5b);
  HAL(writeReg)(TX_CFG_CW_REG, 0x1f); //cw conductance  
  HAL(writeReg)(CODER_CONTROL_REG, 0x19);
  HAL(writeReg)(MOD_WIDTH_REG, 0x0f);//mod width
  HAL(writeReg)(MOD_WIDTH_SOF_REG, 0x0f);  //mod width sof
  //rx session
  HAL(writeReg)(RX_CONTROL1_REG, 0x6b);
  HAL(writeReg)(BPSK_DEM_CONTROL_REG, 0x06);//auto gain
  //HAL(writeReg)(BPSK_DEM_CONTROL_REG, 0x04);
  HAL(writeReg)(DECODER_CONTROL_REG, 0x28);
  HAL(writeReg)(BITPHASE_REG, 0x3d);  //set bitphase
  HAL(writeReg)(RX_THRESHOLD_REG, 0x8c);  //set colllevel, minlevel
  HAL(writeReg)(RX_CONTROL2_REG, 0x41);
  
  //RF-timming channel redundancy
  HAL(writeReg)(RX_WAIT_REG, 0x07);//set rxwait
  HAL(writeReg)(CHANNEL_REDUNDANCY_REG, 0x03);//crc, no parity, preset crc lsb = 0xff, crc msb = 0xff
  HAL(writeReg)(CRC_PRESET_LSB_REG, 0x63);//set rxwait
  HAL(writeReg)(CRC_PRESET_MSB_REG, 0x63);//set rxwait
  congifGain(PLUS0DB); 
}

//make a reqa transaction to 14443a tag
//return atqa
uint16_t SIC9310::reqa(){
  byte txbuf[1];
  byte rxbuf[2];
  uint8_t length;
  HAL(writeReg)(CHANNEL_REDUNDANCY_REG, 0x03);//crc, no parity, preset crc lsb = 0xff, crc msb = 0xff
  HAL(writeReg)(BIT_FRAMING_REG, 7);
  txbuf[0] = 0x26;
  HAL(writeFIFO)(txbuf, 1);
  length = HAL(getFIFOLength)();
  HAL(writeCmd)(TRANSCEIVE_CMD);
  delay(1);
  length = HAL(getFIFOLength)();
  if(length > 0){
    HAL(readFIFO)(rxbuf, length);
    return (rxbuf[1] << 8) | rxbuf[0];
  }else{
    return -1;
  }
}

//make a anti-collision transaction to 14443a tag, this function no implements yet to read multi-tag.
//param: 
//  type, cascade
//return: 
//  0, success
//  -1, false
int SIC9310::antiColl14443A(uint8_t type){
  byte txbuf[7];
  byte rxbuf[5];
  uint8_t length;
  byte controlReg = HAL(readReg)(CONTROL_REG);
  controlReg |= 1;
  HAL(writeReg)(CONTROL_REG, controlReg);
  HAL(writeReg)(CHANNEL_REDUNDANCY_REG, 0x03);//crc, no parity, preset crc lsb = 0xff, crc msb = 0xff
  if(type == 1){
    txbuf[0] = 0x93;
  }else if(type == 2) {
    txbuf[0] = 0x95;
  }
  txbuf[1] = 0x20;   
  HAL(writeFIFO)(txbuf, 2);
  length = HAL(getFIFOLength)();    
  HAL(writeCmd)(TRANSCEIVE_CMD);
  delay(3);
  length = HAL(getFIFOLength)();  
  if(length == 5){
    HAL(readFIFO)(rxbuf, length);
    for(int i = 0; i< length; i++){
      uid_temp[i + ((type - 1) * 5)] = rxbuf[i];
    }    
    return 0;
  }     
  return -1;
}

//make a select transaction to 14443a tag
//param: 
//  type, cascade
//return: 
//  sak, success
//  -1, false
uint8_t SIC9310::select14443A(uint8_t type){
  byte txbuf[7];
  byte rxbuf[1];
  uint8_t length;
  byte contorlReg = HAL(readReg)(CONTROL_REG);
  contorlReg |= 1;
  HAL(writeReg)(CONTROL_REG, contorlReg);
  HAL(writeReg)(CHANNEL_REDUNDANCY_REG, 0x0f);//crc, parity, preset crc lsb = 0xff, crc msb = 0xff
  if(type == 1){
    txbuf[0] = 0x93;
  }else if(type == 2){
    txbuf[0] = 0x95;
  }
  txbuf[1] = 0x70;
  for(int i = 0; i < 5; i++){
    txbuf[2 + i] = uid_temp[i + ((type - 1) * 5)];
  }
  HAL(writeFIFO)(txbuf, 7);
  length = HAL(getFIFOLength)();  
  HAL(writeCmd)(TRANSCEIVE_CMD);
  delay(2);
  length = HAL(getFIFOLength)();  
  if(length == 1){
    HAL(readFIFO)(rxbuf, length);      
    if(type == 1){
      mUidLength = 4;
    }else if(type ==2){
      mUidLength = 7;
    }
    return rxbuf[0];
  }
  return -1;
}

//make a read transaction to 14443a tag
//param: int, byte*
//  page...page address
//  data...4 bytes data
//return: 
//  1, success
//  0, false
uint8_t SIC9310::T2tag_ReadPage(int page, byte* data){
  byte txbuf[2]; 
  byte rxbuf[16];
  uint8_t length;
  byte controlReg = HAL(readReg)(CONTROL_REG);
  controlReg |= 1;
  HAL(writeReg)(CONTROL_REG, controlReg);//flush FIFO
  HAL(writeReg)(CHANNEL_REDUNDANCY_REG, 0x0f);//tx crc, parity
  txbuf[0] = 0x30; txbuf[1] = page;
  HAL(writeFIFO)(txbuf, 2);
  delay(2);
  HAL(writeCmd)(TRANSCEIVE_CMD);
  delay(5);
  length = HAL(getFIFOLength)(); 
  if(length == 16){
    HAL(readFIFO)(rxbuf, length);
    for(int i = 0; i < 4; i++){
      data[i] = rxbuf[i];
    }
    return 1;
  }
  return 0;
}
//make a write transaction to 14443a tag
//param: int, byte*
//  page...page address
//  data...4 bytes data
//return: 
//  1, success
//  0, false
uint8_t SIC9310::T2tag_WritePage(int page, byte* data){
  byte txbuf[6]; 
  byte rxbuf[1];
  
  uint8_t length;
  byte controlReg = HAL(readReg)(CONTROL_REG);
  controlReg |= 1;
  HAL(writeReg)(CONTROL_REG, controlReg);//flush FIFO
  HAL(writeReg)(CHANNEL_REDUNDANCY_REG, 0x07);//crc, parity
  txbuf[0] = 0xa2; txbuf[1] = page;
  for(int i = 0; i < 4; i++){
    txbuf[i + 2] = data[i];
  }
  HAL(writeFIFO)(txbuf, 6);
  delay(2);  
      
  HAL(writeCmd)(TRANSCEIVE_CMD);
  delay(10);
  length = HAL(getFIFOLength)();      

  if(length == 1){
    HAL(readFIFO)(rxbuf, length);          
    if(rxbuf[0] == 0x0a){
      return 1;
    }
  }
  return 0;
}


int SIC9310::getTagId(byte* id){
  if(mUidLength == 4){
    for(int i = 0; i < mUidLength; i++){
      id[i] = uid_temp[i];
    }
    return 4;
  }else if(mUidLength == 7){
    for(int i = 0; i < mUidLength; i++){
      if(i<3){
        id[i] = uid_temp[i + 1];
      }else{
        id[i]=uid_temp[i+2];
      }
    }   
    return 7;
  }
  return 0;
}

//detect tag
//return: boolean
uint8_t SIC9310::isTagPresent(byte * nowUID)
{
  uidLength = 0;
  if(reqa() == 0x44){
    delay(1);
    for(int type = 0; type < 3; type ++){
      if(antiColl14443A(type + 1) != 0){
        break;
      }
      delay(1);
      int sak = select14443A(type + 1);
      if(sak == 0x00){
        uidLength = getTagId(uid);
        bool uid_diff = false;
        for (int i =0; i < uidLength; i++) {
             if ( uid[i] != nowUID[i] ) uid_diff = true;
        }
        if (uid_diff) { // not the same as now
             uid_diff = false;
             for (int i =0; i < uidLength; i++) {
                  if ( uid[i] != prevUID[i] ) uid_diff = true;
             }
             if (uid_diff) { // also not the same as previous
                 uid_compare = false;
                 for (int i =0; i < uidLength; i++) {
                      prevUID[i] = nowUID[i];
                      nowUID[i] = uid[i];
                 }
                 return 1;
             } else {
                 uid_compare = true;
                 return 2;
             }
       } // same 
       return 0;
//        Serial.print("uid --- ");
//        for(int i =0; i < uidLength; i++){
//          Serial.print(", ");
//          Serial.print(uid[i], HEX);
//        }
//        Serial.println();
      } else if (sak == 0x04){
        continue;
      }
    }
  }  
  return 9;
}
void SIC9310::idle(){
  HAL(writeCmd)(IDLE_CMD);
}

byte SIC9310::setTxPwr(byte data){
  if (data == 0x00) {
      return HAL(readReg)(TX_CFG_CW_REG);
  } else if (data > 0x3F) {
      data = 0x3F;
  }
  HAL(writeReg)(TX_CFG_CW_REG, data);
  return HAL(readReg)(TX_CFG_CW_REG);
}

void SIC9310::clearError(){
  Serial.println(HAL(readReg)(PRIMARY_STATUS_REG),BIN);
  Serial.println(HAL(readReg)(ERROR_FLAG_REG),BIN);
  HAL(writeReg)(PRIMARY_STATUS_REG,0x00);
  HAL(writeReg)(ERROR_FLAG_REG, 0x00);
  HAL(writeReg)(CONTROL_REG, 0x01);//flush FIFO
}
