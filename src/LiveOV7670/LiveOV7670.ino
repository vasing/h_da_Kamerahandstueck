

#include <Wire.h>
#include "Adafruit_DRV2605.h"
#include "MyTCA9548A.h"

Adafruit_DRV2605 drv;
MyTCA9548A TCA(0x70);
void DRVcalib(int I2CPort);
void sentGo(int I2CPort);

// change setup.h to switch between buffered and pixel-by-pixel processing
#include "setup.h"



void setup() {
  // This is not necessary and has no effect for ATMEGA based Arduinos.
  // WAVGAT Nano has slower clock rate by default. We want to reset it to maximum speed
  CLKPR = 0x80; // enter clock rate change mode
  CLKPR = 0; // set prescaler to 0. WAVGAT MCU has it 3 by default.

  //initializeScreenAndCamera(); //Tislenko auskom. für LRA Test

  Serial.begin(9600);
  Serial.println("DRV test");

  DRVcalib(7);    //calibration for DVR on TCA port 7
  DRVcalib(6);    //calibration for DVR on TCA port 6

}


void loop() {
  //processFrame();       //Tislenko auskom. für LRA Test

  sentGo(0);      //sent DVR waveform trigger on oprt 7
  delay(1000);
  sentGo(1);      //sent DVR waveform trigger on oprt 6
  delay(1000);
  sentGo(2);      //sent DVR waveform trigger on oprt 6
  delay(1000);
  sentGo(3);      //sent DVR waveform trigger on oprt 6
  delay(1000);

}



void DRVcalib(int I2CPort) {

    TCA.resetChannel();
    TCA.setChannel(I2CPort);

    drv.begin();
    drv.useLRA();
    // I2C trigger by sending 'go' command 
    drv.setMode(DRV2605_MODE_INTTRIG); // default, internal trigger when sending GO command

    drv.selectLibrary(1);
    drv.setWaveform(0, 84);  // ramp up medium 1, see datasheet part 11.2
    drv.setWaveform(1, 1);  // strong click 100%, see datasheet part 11.2
    drv.setWaveform(2, 0);  // end of waveforms
}

void sentGo(int I2CPort) {
    TCA.resetChannel();
    TCA.setChannel(I2CPort);

    drv.begin();
    drv.go();
}

