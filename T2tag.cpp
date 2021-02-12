#include "T2tag.h"

#define ULTRALIGHT_PAGE_SIZE 4
#define ULTRALIGHT_READ_SIZE 4 // we should be able to read 16 bytes at a time

#define ULTRALIGHT_DATA_START_PAGE 4
#define ULTRALIGHT_MESSAGE_LENGTH_INDEX 1
#define ULTRALIGHT_DATA_START_INDEX 2
#define ULTRALIGHT_MAX_PAGE 63
#define LONG_TLV_SIZE 4
#define SHORT_TLV_SIZE 2

#define NFC_FORUM_TAG_TYPE_2 ("NFC Forum Type 2")

T2tag::T2tag(SIC9310& nfcShield)
{
    nfc = &nfcShield;
    ndefStartIndex = 0;
    messageLength = 0;
}

T2tag::~T2tag()
{
}

//read ndef 
int T2tag::read(byte * data)
{
    if (isUnformatted())
    {
        Serial.println(F("WARNING: Tag is not formatted."));
        return 0;
    }
    readCapabilityContainer(); // meta info for tag
    findNdefMessage();
    calculateBufferSize();

    if (messageLength == 0) { // data is 0x44 0x03 0x00 0xFE

        return 0;
    }

    boolean success;
    uint8_t page;
    uint8_t index = 0;
    for (page = ULTRALIGHT_DATA_START_PAGE; page < ULTRALIGHT_MAX_PAGE; page++)
    {
        // read the data
        success = nfc->T2tag_ReadPage(page, data + index);
        if (!success)
        {
            Serial.print(F("Read failed "));Serial.println(page);
            // TODO error handling
            messageLength = 0;
            break;
        }

        if (index >= (messageLength + ndefStartIndex))
        {
            break;
        }

        index += ULTRALIGHT_PAGE_SIZE;
        Serial.print("index: ");
        Serial.println(index);        
    }
    *(data + index + 1) = 0xfe;
    Serial.print(" messageLength + ndefStartIndex: ");
    Serial.println( messageLength + ndefStartIndex);      
    return messageLength + ndefStartIndex +1;
}

//write ndef 
boolean T2tag::write(byte* data, uint8_t len)
{
    // Write to tag
    int index = 0;
    int currentPage = 4;
    Serial.println("");
    while (index < len){
      
      Serial.print(index, HEX);
      for(int i = 0; i < ULTRALIGHT_PAGE_SIZE; i++){
        Serial.print(", ");
        Serial.print(data[i+index],HEX);
      }
      Serial.println();
      
        int write_success = nfc->T2tag_WritePage (currentPage, data + index);
        if (!write_success)
        {
            Serial.print(F("Write failed "));Serial.println(currentPage);
            return false;
        }
        index += ULTRALIGHT_PAGE_SIZE;
        currentPage++;

    }
    Serial.println();
    return true;
}
boolean T2tag::isUnformatted()
{
    uint8_t page = 4;
    byte data[ULTRALIGHT_READ_SIZE];
    boolean success = nfc->T2tag_ReadPage (page, data);
    if (success)
    {
        return (data[0] == 0xFF && data[1] == 0xFF && data[2] == 0xFF && data[3] == 0xFF);
    }
    else
    {
        Serial.print(F("Error. Failed read page "));Serial.println(page);
        return false;
    }
}

// page 3 has tag capabilities
void T2tag::readCapabilityContainer()
{
    byte data[ULTRALIGHT_PAGE_SIZE];
    int success = nfc->T2tag_ReadPage (3, data);
    if (success)
    {
        // See AN1303 - different rules for Mifare Family byte2 = (additional data + 48)/8
        tagCapacity = data[2] * 8;
        #ifdef MIFARE_ULTRALIGHT_DEBUG
        Serial.print(F("Tag capacity "));Serial.print(tagCapacity);Serial.println(F(" bytes"));
        #endif

        // TODO future versions should get lock information
    }
}

// read enough of the message to find the ndef message length
void T2tag::findNdefMessage()
{
    int page;
    byte data[12]; // 3 pages
    byte* data_ptr = data;

    // the nxp read command reads 4 pages, unfortunately adafruit give me one page at a time
    boolean success = true;
    for (page = 4; page < 6; page++)
    {
        success = success && nfc->T2tag_ReadPage(page, data_ptr);
        #ifdef MIFARE_ULTRALIGHT_DEBUG
        Serial.print(F("Page "));Serial.print(page);Serial.print(F(" - "));
//        nfc->PrintHexChar(data_ptr, 4);
        #endif
        data_ptr += ULTRALIGHT_PAGE_SIZE;
    }
    if (success)
    {
        if (data[0] == 0x03)
        {
            messageLength = data[1];
            ndefStartIndex = 2;
        }
        else if (data[5] == 0x3) // page 5 byte 1
        {
            // TODO should really read the lock control TLV to ensure byte[5] is correct
            messageLength = data[6];
            ndefStartIndex = 7;
        }
    }
    #ifdef MIFARE_ULTRALIGHT_DEBUG
    Serial.print(F("messageLength "));Serial.println(messageLength);
    Serial.print(F("ndefStartIndex "));Serial.println(ndefStartIndex);
    #endif
}

// buffer is larger than the message, need to handle some data before and after
// message and need to ensure we read full pages
void T2tag::calculateBufferSize()
{
    // TLV terminator 0xFE is 1 byte
    bufferSize = messageLength + ndefStartIndex + 1;

    if (bufferSize % ULTRALIGHT_READ_SIZE != 0)
    {
        // buffer must be an increment of page size
        bufferSize = ((bufferSize / ULTRALIGHT_READ_SIZE) + 1) * ULTRALIGHT_READ_SIZE;
    }
}
int T2tag::getBufferSize(int messageLength){
    int bufferSize = messageLength;
    // TLV header is 2 or 4 bytes, TLV terminator is 1 byte.
    if (messageLength < 0xFF)    {
        bufferSize += SHORT_TLV_SIZE + 1;
    }    else    {
        bufferSize += LONG_TLV_SIZE + 1;
    }
    // bufferSize needs to be a multiple of BLOCK_SIZE
    if (bufferSize % ULTRALIGHT_PAGE_SIZE != 0)    {
        bufferSize = ((bufferSize / ULTRALIGHT_PAGE_SIZE) + 1) * ULTRALIGHT_PAGE_SIZE;
    }
    return bufferSize;
}
