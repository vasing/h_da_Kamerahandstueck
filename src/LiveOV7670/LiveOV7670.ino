

#include <Wire.h>


#include "setup.h"


void setup() {
  // This is not necessary and has no effect for ATMEGA based Arduinos.
  // WAVGAT Nano has slower clock rate by default. We want to reset it to maximum speed
  CLKPR = 0x80; // enter clock rate change mode
  CLKPR = 0; // set prescaler to 0. WAVGAT MCU has it 3 by default.

  initializeScreenAndCamera(); //Tislenko auskom. f�r LRA Test

  //Serial.begin(9600);
  


}


void loop() {
    //int FrameNumber_T = 5;

    //for (int i = 0; i < FrameNumber_T; i++) {
        processFrame();       //Tislenko auskom. fuer LRA Test
    //    }
    
    //Serial.println("loop");

}




