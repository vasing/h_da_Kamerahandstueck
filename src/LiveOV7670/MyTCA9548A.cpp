//MyTCA9548A.cpp
//Code fuer Arduino
//Author Retian
//Version 1.0


/*
Ansteuerung des 8-Kanal I2C-Multiplexer TCA9548A

MyTCA9548A Name(I2C-Adresse);

Beispiel siehe unter:
http://arduino-projekte.webnode.at/meine-libraries/i2c-multiplexer-tca9548a/

Funktionen siehe unter:
http://arduino-projekte.webnode.at/meine-libraries/i2c-multiplexer-tca9548a/funktionen/

*/

//*************************************************************************
//*************************************************************************

#include "Arduino.h"
#include "Wire.h"
#include "MyTCA9548A.h"

MyTCA9548A::MyTCA9548A(byte i2CAdd)
{
    _i2CAdd = i2CAdd;
    Wire.begin();
}


//*************************************************************************
//TCA9548A vorhanden ?

bool MyTCA9548A::isReady()
{
    Wire.beginTransmission(_i2CAdd);
    if (Wire.endTransmission() == 0) return true;
    else return false;
}


//*************************************************************************
//Definieren, welche Kanaele beschaltet sind

void MyTCA9548A::defineWiredChannel(bool ch0, bool ch1, bool ch2, bool ch3,
    bool ch4, bool ch5, bool ch6, bool ch7)
{
    _wiredChannel[0] = ch0;
    _wiredChannel[1] = ch1;
    _wiredChannel[2] = ch2;
    _wiredChannel[3] = ch3;
    _wiredChannel[4] = ch4;
    _wiredChannel[5] = ch5;
    _wiredChannel[6] = ch6;
    _wiredChannel[7] = ch7;
}


//*************************************************************************
//Kanal auswählen

bool MyTCA9548A::setChannel(byte channel)
{
    byte _channel = channel;

    if (_channel < 8 && _wiredChannel[_channel])
    {
        Wire.beginTransmission(_i2CAdd);
        Wire.write(1 << _channel);
        Wire.endTransmission();
        return true;
    }
    else return false;
}


//*************************************************************************
//Kanal-Auswahl rücksetzen

void MyTCA9548A::resetChannel()
{
    Wire.beginTransmission(_i2CAdd);
    Wire.write(0);
    Wire.endTransmission();
}


//*************************************************************************
//Kanal-Auswahl ruecklesen

byte MyTCA9548A::getChannel()
{
    byte inByte;
    byte z = 0;

    Wire.requestFrom(int(_i2CAdd), 1);
    while (Wire.available() == 0);
    inByte = (Wire.read());
    if (inByte == 0) return 255;
    else
    {
        while (inByte > 1)
        {
            inByte = (inByte >> 1);
            z++;
        }
    }
    return z;
}






