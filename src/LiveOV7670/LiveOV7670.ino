

#include <Wire.h>
#include "Adafruit_DRV2605.h"
#include "MyTCA9548A.h"

Adafruit_DRV2605 drv;
MyTCA9548A TCA(0x70);
void DRVcalib(int I2CPort);
void sentGo(int I2CPort);

// change setup.h to switch between buffered and pixel-by-pixel processing
#include "setup.h"

//Tislenko
#define LRA_SW 1        //0 - Schaltet LRA Ansteuerung aus
                        //1 - Schaltet LRA Ansteuerung ein
#define UART_INIT 0     //0 - Schaltet UART ArduinoCapture aus
                        //1 - Schaltet UART ArduinoCapture ein
#define UART_OUTPUT 0   //0 - Schaltet UART Output durch ArduinoCapture aus
                        //1 - Schaltet UART Output durch ArduinoCapture ein


int MIN_LINE_WIDTH = 10;
int MAX_LINE_WIDTH = 100;

uint16_t DetectLineWidth();
void ResetData();


void setup() {
  // This is not necessary and has no effect for ATMEGA based Arduinos.
  // WAVGAT Nano has slower clock rate by default. We want to reset it to maximum speed
  CLKPR = 0x80; // enter clock rate change mode
  CLKPR = 0; // set prescaler to 0. WAVGAT MCU has it 3 by default.

  initializeScreenAndCamera(); //Tislenko auskom. f�r LRA Test

  //Serial.begin(9600);
  

  DRVcalib(0);    //calibration for DVR on TCA port 0
  DRVcalib(1);    //calibration for DVR on TCA port 1
  DRVcalib(2);    //calibration for DVR on TCA port 2
  DRVcalib(3);    //calibration for DVR on TCA port 3
}


void loop() {
    //int FrameNumber_T = 5;

    //for (int i = 0; i < FrameNumber_T; i++) {
        processFrame();       //Tislenko auskom. fuer LRA Test
    //    }
    
    //Serial.println("loop");

#if LRA_SW==1
    
    
    if ((DetectLineWidth > MIN_LINE_WIDTH) && (DetectLineWidth > MAX_LINE_WIDTH)) {
        sentGo(0);
    }
    
    //Serial.println("link");
    sentGo(0);      //LRA "links" (Zeigefinger)
    //delay(1000);
    //Serial.println("unten");
    
    //sentGo(1);      //sent DVR waveform trigger on oprt 6
    //delay(1000);
    //Serial.println("rechts");
    
    //sentGo(2);      //sent DVR waveform trigger on oprt 6
    //delay(1000);
    //Serial.println("oben");
    
    //sentGo(3);      //sent DVR waveform trigger on oprt 6
    //delay(1000);
#endif

/*
#if UART_ONLY==1
  Serial.println("link");
  sentGo(0);      //sent DVR waveform trigger on oprt 7
  delay(1000);
  Serial.println("unten");
  sentGo(1);      //sent DVR waveform trigger on oprt 6
  delay(1000);
  Serial.println("rechts");
  sentGo(2);      //sent DVR waveform trigger on oprt 6
  delay(1000);
  Serial.println("oben");
  sentGo(3);      //sent DVR waveform trigger on oprt 6
  delay(1000);
#endif
*/
}



void DRVcalib(int I2CPort) {

    TCA.resetChannel();
    TCA.setChannel(I2CPort);

    drv.begin();
    drv.useLRA();
    //drv.useERM();
    // I2C trigger by sending 'go' command 
    drv.setMode(DRV2605_MODE_INTTRIG); // default, internal trigger when sending GO command

    drv.selectLibrary(1);
    drv.setWaveform(0, 82);  // ramp up medium 1, see datasheet part 11.2
    drv.setWaveform(1, 1);  // strong click 100%, see datasheet part 11.2
    drv.setWaveform(2, 0);  // end of waveforms
}

void sentGo(int I2CPort) {
    TCA.resetChannel();
    TCA.setChannel(I2CPort);

    drv.begin();
    drv.go();
}

uint16_t DetectLineWidth() {

    uint8_t LineWidth = 0;

    //uint8_t gap = AllowedGap;

    for (int i = 0; i <= 160; i++) {            //#Durchlaufe Frame Linie
        if (LINE_0_FRAME[i] == 1) {              //#Suche erstes Pixel
            //FirstDetected = true;               //#DetectLine Start
            LineWidth++;                        //#zähle Linienbreite
        }
    }
    return LineWidth;
}

void ResetData() {

    for (int i = 0; i <= 159; i++) {
        LINE_0_FRAME[i] = 0;
        LINE_1_FRAME[i] = 0;
        LINE_RESULT[i] = 0;
    }
}
