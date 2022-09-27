//MyTCA9548A

#ifndef MyTCA9548A_h
#define MyTCA9548A_h

#include "Arduino.h"

class MyTCA9548A
{
public:
    MyTCA9548A(byte);
    bool isReady(void);
    void defineWiredChannel(bool, bool, bool, bool, bool, bool, bool, bool);
    bool setChannel(byte);
    void resetChannel(void);
    byte getChannel(void);


private:
    byte _i2CAdd;
    bool _wiredChannel[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };
};

#endif

