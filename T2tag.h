#ifndef T2tag_h
#define T2tag_h
#include "SIC9310.h"
class T2tag
{
    public:
        T2tag(SIC9310& nfcShield);
        ~T2tag();
        int read(byte *data);
        boolean write(byte * data, uint8_t len);
    private:
        SIC9310* nfc;
        unsigned int tagCapacity;
        unsigned int messageLength;
        unsigned int bufferSize;
        unsigned int ndefStartIndex;
        boolean isUnformatted();
        void readCapabilityContainer();
        void findNdefMessage();
        void calculateBufferSize();
        int getBufferSize(int messageLength);
};

#endif
