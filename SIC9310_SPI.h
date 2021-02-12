#ifndef __SIC9310_SPI_
#define __SIC9310_SPI_
#include <Arduino.h>
#include <SPI.h>

#define DEVICE_TYPE_REG 0x38
#define COMMAND_REG 0x01
#define FIFO_DATA_REG 0x02
#define PRIMARY_STATUS_REG 0x03
#define FIFO_LENGTH_REG 0x04
#define SECONDARY_STATUS_REG 0x05
#define INTERRUPT_ENABLE_REG 0x06
#define INTERRUPT_FLAG_REG 0x07
#define CONTROL_REG 0x09
#define ERROR_FLAG_REG 0x0a
#define COLL_POS_REG 0x0b
#define TIMER_VALUE_REG 0x0c
#define CRC_RESULT_LSB_REG 0x0d
#define CRC_RESULT_MSB_REG 0x0e
#define BIT_FRAMING_REG 0x0f
#define TX_CONTROL_REG 0x11
#define TX_CFG_CW_REG 0x12
#define TX_CFG_MOD_REG 0X13
#define CODER_CONTROL_REG 0X14
#define MOD_WIDTH_REG 0x15
#define MOD_WIDTH_SOF_REG 0x16
#define TYPEB_FRAMING_REG 0x17
#define RX_CONTROL1_REG 0x19
#define DECODER_CONTROL_REG 0x1a
#define BITPHASE_REG 0X1b
#define RX_THRESHOLD_REG 0X1c
#define BPSK_DEM_CONTROL_REG 0x1d
#define RX_CONTROL2_REG 0x1e
#define RX_CONTROL3_REG 0x1f
#define RX_WAIT_REG 0x21
#define CHANNEL_REDUNDANCY_REG 0x22
#define CRC_PRESET_LSB_REG 0x23
#define CRC_PRESET_MSB_REG 0x24
#define TIMER_CLOCK_REG 0x2a
#define TIMER_RELOAD_VALUE_REG 0x2c
#define ANALOG_ADJUST1_REG 0x3e
#define ANALOG_ADJUST2_REG 0x3f
#define GAIN_ST3 0x3f
#define ANALOG_ADJUST3_REG 0x3c
//---- For ISO15693----------------//
// Speed Tx
#define	 SPEED_1_OUT_OF_256		0x00
#define	 SPEED_1_OUT_OF_4		0x01
// Speed Rx
#define	 SPEED_1_SUB_LOW		0x00
#define	 SPEED_1_SUB_HIGH		0x01
#define	 SPEED_1_SUB_ULTRA_HIGH	0x02
#define	 SPEED_2_SUB_LOW		0x03
#define	 SPEED_2_SUB_HIGH		0x04
//******* Mask for TxControl Reg (Reg 0x11) *************************//
#define	 ModSource_Clear_Mask	0x9F
#define	 Force100ASK_Set_Mask	0x10
#define	 Force100ASK_Clear_Mask	0xEF
#define	 TX2RFEn_Set_Mask		0x02
#define	 TX2RFEn_Clear_Mask		0xFD
#define	 TX1RFEn_Set_Mask		0x01
#define	 TX1RFEn_Clear_Mask		0xFE
#define	 TX2Inv_Clear_Mask		0xF7
#define	 TX2Cw_Set_Mask			0x04
#define	 TX2Cw_Clear_Mask		0xFB

//sic9310 command
#define IDLE_CMD 0x00
#define TRANSMIT_CMD 0x1a
#define RECEVIE_CMD 0x16
#define TRANSCEIVE_CMD 0x1e
#define WIRTE_EEPROM_CMD 0x01
#define READ_EEPROM_CMD 0x03
#define LOAD_CONFIG_E2_CMD 0x07
#define CALCULATE_CRC_CMD 0x12
#define LOAD_KEY_EEPROM_CMD 0x0b
#define LOAD_KEY_FIFO_CMD 0x19
#define AUTHENT_CMD 0x1c
#define TUNE_FILTER_CMD 0x10
//
#define MOD_SOURZCE_INTERNAL 0x02

#define DRIVER_CONFIG_X_E50OUT 50
#define FORCE100ASK_SET 0x10
class SIC9310_SPI{
public:  
  SIC9310_SPI(SPIClass &spi, uint8_t ss);
  void begin();
  void writeReg(byte addr, byte data);
  void writeCmd(byte cmd);
  void writeReg(byte addr, byte* data, uint8_t len);
  void writeFIFO(byte* data, uint8_t len);
  byte readReg(byte addr);
  void readFIFO(byte* data, uint8_t len);
  uint8_t getFIFOLength();  
private:
  uint8_t _ss;
  SPIClass* _spi;
  inline void write(uint8_t data) {
        _spi->transfer(data);
  };
  inline byte read(byte addr) {          
      return _spi->transfer(addr);         
  };   

};
#endif
