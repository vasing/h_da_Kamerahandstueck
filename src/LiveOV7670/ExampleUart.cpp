//
// Created by indrek on 1.05.2016.
//

// set EXAMPLE to EXAMPLE_UART in setup.h to activate
#include "setup.h"
#if EXAMPLE == 3
#include "Arduino.h"
#include "CameraOV7670.h"
#include "Adafruit_DRV2605.h"
#include "MyTCA9548A.h"


#define CONTRAST 255    //0...255 Setzt OV7670 Kontrast. 
                        //64 = default
#define BRIGHTNESS 32   //0...255 Setzt OV7670 Helligheit
                        //0 = default
//Tislenko: Parameter fuer Graustufen Limit in Funktion formatPixelByteBinary()
const uint8_t BinaryLimit = 150; // Graustufen Limit



Adafruit_DRV2605 drv;
MyTCA9548A TCA(0x70);
void DRVcalib(int I2CPort);
void sentGo(int I2CPort);

// change setup.h to switch between buffered and pixel-by-pixel processing


//Tislenko
#define LRA_SW 1        //0 - Schaltet LRA Ansteuerung aus
                        //1 - Schaltet LRA Ansteuerung ein
#define UART_INIT 0     //0 - Schaltet UART ArduinoCapture aus
                        //1 - Schaltet UART ArduinoCapture ein
#define UART_OUTPUT 0   //0 - Schaltet UART Output durch ArduinoCapture aus
                        //1 - Schaltet UART Output durch ArduinoCapture ein


uint16_t MIN_LINE_WIDTH = 10;
uint16_t MAX_LINE_WIDTH = 100;

uint16_t DetectLineWidth();
void ResetData();


// select resolution and communication speed:
//  1 - 115200bps 160x120 rgb
//  2 - 115200bps 160x120 grayscale
//  3 - 500000bps 160x120 rgb
//  4 - 500000bps 160x120 grayscale
//  5 - 500000bps 320x240 rgb
//  6 - 500000bps 320x240 grayscale
//  7 - 1Mbps 160x120 rgb
//  8 - 1Mbps 160x120 grayscale
//  9 - 1Mbps 320x240 rgb
// 10 - 1Mbps 320x240 grayscale
// 11 - 1Mbps 640x480 grayscale
// 12 - 2Mbps 160x120 rgb
// 13 - 2Mbps 160x120 grayscale
// 14 - 2Mbps 320x240 rgb
// 15 - 2Mbps 320x240 grayscale
// 16 - 2Mbps 640x480 rgb
// 17 - 2Mbps 640x480 grayscale
#define UART_MODE 4




//Nutzdaten (Anzahl Pixel in horiz. Linien): 160 bei UART MODE 1,2,7,8,12,13
//0b0000000 ist Fehlerwert
//groesser 0b10100000 (160) ist nicht plausibel
uint8_t LINE_0 = 0b0000000; 
uint8_t LINE_1 = 0b0000000; 
uint8_t LINE_2 = 0b0000000; 
uint8_t LINE_3 = 0b0000000;

//Nutzdaten (Anzahl Pixel in vert. Linien): 120  UART MODE 8 1,2,7,8,12,13
//0b0000000 ist Fehlerwert
//groesser 0b01111000 (120) ist nicht plausibel
uint8_t ROW_0 = 0b00000000;
uint8_t ROW_1 = 0b00000000;
uint8_t ROW_2 = 0b00000000;
uint8_t ROW_3 = 0b00000000;



int LINE_0_FRAME[159];      //Reihe 1, Frame 1
int LINE_1_FRAME[159];      //Reihe 2, Frame 1
int ROW_0_FRAME[119];       //Spalte 1, Frame 1
int ROW_1_FRAME[119];       //Spalte 1, Frame 1

const uint8_t VERSION = 0x10;
const uint8_t COMMAND_NEW_FRAME = 0x01 | VERSION;
const uint8_t COMMAND_DEBUG_DATA = 0x03 | VERSION;

const uint16_t UART_PIXEL_FORMAT_RGB565 = 0x01;
const uint16_t UART_PIXEL_FORMAT_GRAYSCALE = 0x02;

// Pixel byte parity check:
// Pixel Byte H: odd number of bits under H_BYTE_PARITY_CHECK and H_BYTE_PARITY_INVERT
// Pixel Byte L: even number of bits under L_BYTE_PARITY_CHECK and L_BYTE_PARITY_INVERT
//                                          H:RRRRRGGG
const uint8_t H_BYTE_PARITY_CHECK = 0b00100000;
const uint8_t H_BYTE_PARITY_INVERT = 0b00001000;
//                                          L:GGGBBBBB
const uint8_t L_BYTE_PARITY_CHECK = 0b00001000;
const uint8_t L_BYTE_PARITY_INVERT = 0b00100000;
// Since the parity for L byte can be zero we must ensure that the total byet value is above zero.
// Increasing the lowest bit of blue color is OK for that.
const uint8_t L_BYTE_PREVENT_ZERO = 0b00000001;


const uint16_t COLOR_GREEN = 0x07E0;
const uint16_t COLOR_RED = 0xF800;



void processGrayscaleFrameBuffered();
void processGrayscaleFrameDirect();
void processRgbFrameBuffered();
void processRgbFrameDirect();
typedef void (*ProcessFrameData)(void);


#if UART_MODE==1
const uint16_t lineLength = 160;
const uint16_t lineCount = 120;
const uint32_t baud = 115200;
const ProcessFrameData processFrameData = processRgbFrameBuffered;
const uint16_t lineBufferLength = lineLength * 2;
const bool isSendWhileBuffering = true;
const uint8_t uartPixelFormat = UART_PIXEL_FORMAT_RGB565;
CameraOV7670 camera(CameraOV7670::RESOLUTION_QQVGA_160x120, CameraOV7670::PIXEL_RGB565, 34);
#endif

#if UART_MODE==2
const uint16_t lineLength = 160;
const uint16_t lineCount = 120;
const uint32_t baud = 115200;
const ProcessFrameData processFrameData = processGrayscaleFrameBuffered;
const uint16_t lineBufferLength = lineLength;
const bool isSendWhileBuffering = true;
const uint8_t uartPixelFormat = UART_PIXEL_FORMAT_GRAYSCALE;
CameraOV7670 camera(CameraOV7670::RESOLUTION_QQVGA_160x120, CameraOV7670::PIXEL_YUV422, 17);
#endif

#if UART_MODE==3
const uint16_t lineLength = 160;
const uint16_t lineCount = 120;
const uint32_t baud = 500000;
const ProcessFrameData processFrameData = processRgbFrameBuffered;
const uint16_t lineBufferLength = lineLength * 2;
const bool isSendWhileBuffering = true;
const uint8_t uartPixelFormat = UART_PIXEL_FORMAT_RGB565;
CameraOV7670 camera(CameraOV7670::RESOLUTION_QQVGA_160x120, CameraOV7670::PIXEL_RGB565, 8);
#endif

#if UART_MODE==4
const uint16_t lineLength = 160;
const uint16_t lineCount = 120;
const uint32_t baud = 500000;
const ProcessFrameData processFrameData = processGrayscaleFrameBuffered;
const uint16_t lineBufferLength = lineLength;
const bool isSendWhileBuffering = true;
const uint8_t uartPixelFormat = UART_PIXEL_FORMAT_GRAYSCALE;
CameraOV7670 camera(CameraOV7670::RESOLUTION_QQVGA_160x120, CameraOV7670::PIXEL_YUV422, 4);
#endif

#if UART_MODE==5
const uint16_t lineLength = 320;
const uint16_t lineCount = 240;
const uint32_t baud = 500000;
const ProcessFrameData processFrameData = processRgbFrameBuffered;
const uint16_t lineBufferLength = lineLength * 2;
const bool isSendWhileBuffering = true;
const uint8_t uartPixelFormat = UART_PIXEL_FORMAT_RGB565;
CameraOV7670 camera(CameraOV7670::RESOLUTION_QVGA_320x240, CameraOV7670::PIXEL_RGB565, 32);
#endif

#if UART_MODE==6
const uint16_t lineLength = 320;
const uint16_t lineCount = 240;
const uint32_t baud = 500000;
const ProcessFrameData processFrameData = processGrayscaleFrameBuffered;
const uint16_t lineBufferLength = lineLength;
const bool isSendWhileBuffering = true;
const uint8_t uartPixelFormat = UART_PIXEL_FORMAT_GRAYSCALE;
CameraOV7670 camera(CameraOV7670::RESOLUTION_QVGA_320x240, CameraOV7670::PIXEL_YUV422, 16);
#endif

#if UART_MODE==7
const uint16_t lineLength = 160;
const uint16_t lineCount = 120;
const uint32_t baud = 1000000;
const ProcessFrameData processFrameData = processRgbFrameBuffered;
const uint16_t lineBufferLength = lineLength * 2;
const bool isSendWhileBuffering = false;
const uint8_t uartPixelFormat = UART_PIXEL_FORMAT_RGB565;
CameraOV7670 camera(CameraOV7670::RESOLUTION_QQVGA_160x120, CameraOV7670::PIXEL_RGB565, 5);
#endif

#if UART_MODE==8
const uint16_t lineLength = 160;
const uint16_t lineCount = 120;
const uint32_t baud = 1000000;
const ProcessFrameData processFrameData = processGrayscaleFrameBuffered;
const uint16_t lineBufferLength = lineLength;
const bool isSendWhileBuffering = false;
const uint8_t uartPixelFormat = UART_PIXEL_FORMAT_GRAYSCALE;
CameraOV7670 camera(CameraOV7670::RESOLUTION_QQVGA_160x120, CameraOV7670::PIXEL_YUV422, 2);
#endif

#if UART_MODE==9
const uint16_t lineLength = 320;
const uint16_t lineCount = 240;
const uint32_t baud = 1000000;
const ProcessFrameData processFrameData = processRgbFrameBuffered;
const uint16_t lineBufferLength = lineLength * 2;
const bool isSendWhileBuffering = true;
const uint8_t uartPixelFormat = UART_PIXEL_FORMAT_RGB565;
CameraOV7670 camera(CameraOV7670::RESOLUTION_QVGA_320x240, CameraOV7670::PIXEL_RGB565, 16);
#endif

#if UART_MODE==10
const uint16_t lineLength = 320;
const uint16_t lineCount = 240;
const uint32_t baud = 1000000;
const ProcessFrameData processFrameData = processGrayscaleFrameBuffered;
const uint16_t lineBufferLength = lineLength;
const bool isSendWhileBuffering = true;
const uint8_t uartPixelFormat = UART_PIXEL_FORMAT_GRAYSCALE;
CameraOV7670 camera(CameraOV7670::RESOLUTION_QVGA_320x240, CameraOV7670::PIXEL_YUV422, 8);
#endif

#if UART_MODE==11
const uint16_t lineLength = 640;
const uint16_t lineCount = 480;
const uint32_t baud = 1000000;
const ProcessFrameData processFrameData = processGrayscaleFrameDirect;
const uint16_t lineBufferLength = 1;
const bool isSendWhileBuffering = true;
const uint8_t uartPixelFormat = UART_PIXEL_FORMAT_GRAYSCALE;
CameraOV7670 camera(CameraOV7670::RESOLUTION_VGA_640x480, CameraOV7670::PIXEL_YUV422, 39);
#endif

#if UART_MODE==12
const uint16_t lineLength = 160;
const uint16_t lineCount = 120;
const uint32_t baud = 2000000;
const ProcessFrameData processFrameData = processRgbFrameBuffered;
const uint16_t lineBufferLength = lineLength * 2;
const bool isSendWhileBuffering = false;
const uint8_t uartPixelFormat = UART_PIXEL_FORMAT_RGB565;
CameraOV7670 camera(CameraOV7670::RESOLUTION_QQVGA_160x120, CameraOV7670::PIXEL_RGB565, 2);
#endif

#if UART_MODE==13
const uint16_t lineLength = 160;
const uint16_t lineCount = 120;
const uint32_t baud = 2000000;
const ProcessFrameData processFrameData = processGrayscaleFrameBuffered;
const uint16_t lineBufferLength = lineLength;
const bool isSendWhileBuffering = false;
const uint8_t uartPixelFormat = UART_PIXEL_FORMAT_GRAYSCALE;
CameraOV7670 camera(CameraOV7670::RESOLUTION_QQVGA_160x120, CameraOV7670::PIXEL_YUV422, 2);
#endif

#if UART_MODE==14
const uint16_t lineLength = 320;
const uint16_t lineCount = 240;
const uint32_t baud = 2000000; // may be unreliable
const ProcessFrameData processFrameData = processRgbFrameBuffered;
const uint16_t lineBufferLength = lineLength * 2;
const bool isSendWhileBuffering = true;
const uint8_t uartPixelFormat = UART_PIXEL_FORMAT_RGB565;
CameraOV7670 camera(CameraOV7670::RESOLUTION_QVGA_320x240, CameraOV7670::PIXEL_RGB565, 12);
#endif

#if UART_MODE==15
const uint16_t lineLength = 320;
const uint16_t lineCount = 240;
const uint32_t baud = 2000000; // may be unreliable
const ProcessFrameData processFrameData = processGrayscaleFrameBuffered;
const uint16_t lineBufferLength = lineLength;
const bool isSendWhileBuffering = false;
const uint8_t uartPixelFormat = UART_PIXEL_FORMAT_GRAYSCALE;
CameraOV7670 camera(CameraOV7670::RESOLUTION_QVGA_320x240, CameraOV7670::PIXEL_YUV422, 6);
#endif

#if UART_MODE==16
const uint16_t lineLength = 640;
const uint16_t lineCount = 480;
const uint32_t baud = 2000000;
const ProcessFrameData processFrameData = processRgbFrameDirect;
const uint16_t lineBufferLength = 1;
const bool isSendWhileBuffering = true;
const uint8_t uartPixelFormat = UART_PIXEL_FORMAT_RGB565;
CameraOV7670 camera(CameraOV7670::RESOLUTION_VGA_640x480, CameraOV7670::PIXEL_RGB565, 39);
#endif

#if UART_MODE==17
const uint16_t lineLength = 640;
const uint16_t lineCount = 480;
const uint32_t baud = 2000000;
const ProcessFrameData processFrameData = processGrayscaleFrameDirect;
const uint16_t lineBufferLength = 1;
const bool isSendWhileBuffering = true;
const uint8_t uartPixelFormat = UART_PIXEL_FORMAT_GRAYSCALE;
CameraOV7670 camera(CameraOV7670::RESOLUTION_VGA_640x480, CameraOV7670::PIXEL_YUV422, 19);
#endif

//Tislenko: Eigener UART MODE 
#if UART_MODE==18
const uint16_t lineLength = 160;
const uint16_t lineCount = 120;
const uint32_t baud = 9600;
const ProcessFrameData processFrameData = processGrayscaleFrameDirect;
const uint16_t lineBufferLength = 1;
const bool isSendWhileBuffering = true;
const uint8_t uartPixelFormat = UART_PIXEL_FORMAT_GRAYSCALE;
CameraOV7670 camera(CameraOV7670::RESOLUTION_QQVGA_160x120, CameraOV7670::PIXEL_YUV422, 2);
//Tislenko: Kontrast und Helligkeit Voreinstellungen
#endif

uint8_t lineBuffer[lineBufferLength]; // Two bytes per pixel
uint8_t* lineBufferSendByte;
bool isLineBufferSendHighByte;
bool isLineBufferByteFormatted;

uint16_t frameCounter = 0;
uint16_t processedByteCountDuringCameraRead = 0;


void commandStartNewFrame(uint8_t pixelFormat);
void commandDebugPrint(const String debugText);
uint8_t sendNextCommandByte(uint8_t checksum, uint8_t commandByte);

void sendBlankFrame(uint16_t color);
inline void processNextGrayscalePixelByteInBuffer() __attribute__((always_inline));
inline void processNextRgbPixelByteInBuffer() __attribute__((always_inline));
inline void tryToSendNextRgbPixelByteInBuffer() __attribute__((always_inline));
inline void formatNextRgbPixelByteInBuffer() __attribute__((always_inline));
inline uint8_t formatRgbPixelByteH(uint8_t byte) __attribute__((always_inline));
inline uint8_t formatRgbPixelByteL(uint8_t byte) __attribute__((always_inline));
inline uint8_t formatPixelByteGrayscaleFirst(uint8_t byte) __attribute__((always_inline));
inline uint8_t formatPixelByteBinary(uint8_t byte) __attribute__((always_inline));
inline void setBinaryResult(uint8_t value, uint16_t nr) __attribute__((always_inline));
inline uint8_t formatPixelByteGrayscaleSecond(uint8_t byte) __attribute__((always_inline));
inline void waitForPreviousUartByteToBeSent() __attribute__((always_inline));
inline bool isUartReady() __attribute__((always_inline));



// this is called in Arduino setup() function
void initializeScreenAndCamera() {

    // Enable this for WAVGAT CPUs
    // For UART communiation we want to set WAVGAT Nano to 16Mhz to match Atmel based Arduino
    //CLKPR = 0x80; // enter clock rate change mode
    //CLKPR = 1; // set prescaler to 1. WAVGAT MCU has it 3 by default.

    Serial.begin(baud);


    if (camera.init()) {    //OV7670 initialisieren
//Konfigurierbar zwischen Normalbetrieb und ArduinoCapture Mode
#if UART_INIT==1            
        sendBlankFrame(COLOR_GREEN);
        delay(1000);
    }
    else {
        //todo Konfigurierbar machen zwischen Normalbetrieb und ArduinoCapture
        sendBlankFrame(COLOR_RED);
        delay(3000);
#endif
        
//Wenn Kontrast manuell definiert und nicht auskommentiert
//#ifdef CONTRAST
        camera.setContrast(CONTRAST);
//#endif

//Wenn Helligkeit manuell definier und nicht auskommentiert
//#ifdef BRIGHTNESS
        camera.setBrightness(BRIGHTNESS);
//#endif

        //Delay, sonnst ist if() leer
        delay(100);

        DRVcalib(0);    //calibration for DVR on TCA port 0
        DRVcalib(1);    //calibration for DVR on TCA port 1
        DRVcalib(2);    //calibration for DVR on TCA port 2
        DRVcalib(3);    //calibration for DVR on TCA port 3

    }

}


void sendBlankFrame(uint16_t color) {
    uint8_t colorH = (color >> 8) & 0xFF;
    uint8_t colorL = color & 0xFF;

    commandStartNewFrame(UART_PIXEL_FORMAT_RGB565);
    for (uint16_t j = 0; j < lineCount; j++) {
        for (uint16_t i = 0; i < lineLength; i++) {
            waitForPreviousUartByteToBeSent();
            UDR0 = formatRgbPixelByteH(colorH);
            waitForPreviousUartByteToBeSent();
            UDR0 = formatRgbPixelByteL(colorL);
        }
    }
}




// this is called in Arduino loop() function
void processFrame() {
    

    
    processedByteCountDuringCameraRead = 0;
    //todo Konfigurierbar machen zwischen Normalbetrieb und ArduinoCapture
    commandStartNewFrame(uartPixelFormat);
    noInterrupts();
    processFrameData();
    interrupts();
    frameCounter++;
    commandDebugPrint("Frame " + String(frameCounter)/* + " " + String(processedByteCountDuringCameraRead)*/);
    
    Serial.println("Frame: ");
    Serial.println(frameCounter);
    
    //commandDebugPrint("Frame " + String(frameCounter, 16)); // send number in hexadecimal

#if LRA_SW==1


    //if ((DetectLineWidth() > MIN_LINE_WIDTH) && (DetectLineWidth() > MAX_LINE_WIDTH)) {
    //    sentGo(0);
    //}

    //Serial.println("link");
    //sentGo(0);      //LRA "links" (Zeigefinger)
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

    /**/
    //#if UART_ONLY==1
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
    //#endif
    


}


void processGrayscaleFrameBuffered() {
    camera.waitForVsync();
    commandDebugPrint("Vsync");

    camera.ignoreVerticalPadding();
    //Hier wird jede Reihe (Linie) durchlaufen

    for (uint16_t y = 0; y < lineCount; y++) {  
        
        //lineBufferSendByte = &lineBuffer[0];
        camera.ignoreHorizontalPaddingLeft();

        //Hier wird jede Spalte (Pixel) durchlaufen
        //todo Hier auch Datenerfassung in Abhaengigkeit vom Pixel
        uint16_t x = 0;
        while (x < lineBufferLength) {       
            
            camera.waitForPixelClockRisingEdge(); // YUV422 grayscale byte
            camera.readPixelByte(lineBuffer[x]);    //readPixelByte() schreibt in den Speicher von lineBuffer[x] die Pixel Bytes
            lineBuffer[x] = formatPixelByteGrayscaleFirst(lineBuffer[x]);
            //lineBuffer[x] = formatPixelByteBinary(lineBuffer[x]); //Tislenko
            
            if (y==60) setBinaryResult(lineBuffer[x], x);       //Tislenko: Linien binaer auswerten

            camera.waitForPixelClockRisingEdge(); // YUV422 color byte. Ignore.
           
            if (isSendWhileBuffering) {     //false für UART MODE 8
                processNextGrayscalePixelByteInBuffer();
            }

            x++;
            /**/
            camera.waitForPixelClockRisingEdge(); // YUV422 grayscale byte
            camera.readPixelByte(lineBuffer[x]);
            lineBuffer[x] = formatPixelByteGrayscaleSecond(lineBuffer[x]);
            //lineBuffer[x] = formatPixelByteBinary(lineBuffer[x]); //Tislenko
            
            if (y == 60) setBinaryResult(lineBuffer[x], x);     //Tislenko: Linien binaer auswerten
            
            camera.waitForPixelClockRisingEdge(); // YUV422 color byte. Ignore.
            
            if (isSendWhileBuffering) {
                processNextGrayscalePixelByteInBuffer();
            }
            
            x++;     
        }   
        camera.ignoreHorizontalPaddingRight();





        // Debug info to get some feedback how mutch data was processed during line read.
        //processedByteCountDuringCameraRead = lineBufferSendByte - (&lineBuffer[0]);
        /*
        // Send rest of the line
        
        /*
        while (lineBufferSendByte < &lineBuffer[lineLength]) {
            processNextGrayscalePixelByteInBuffer();
        }
        */
        //todo Hier Datenerfassung in Abhaengigkeit der Linie

    };
    
    /* Ausgewertete Linien Seriell ausgeben 
    for (uint16_t k = 0; k < 160; k++) {
        Serial.println("Linie:");
        Serial.println(LINE_0_FRAME[k]);
    }
    */
}

void processNextGrayscalePixelByteInBuffer() {
    if (isUartReady()) {
        UDR0 = *lineBufferSendByte;
        lineBufferSendByte++;
    }
}


void processGrayscaleFrameDirect() {
    //uint8_t setContrast = 0x80;
    //camera.setContrast(setContrast);
    camera.waitForVsync();
    commandDebugPrint("Vsync");

    camera.ignoreVerticalPadding();

    for (uint16_t y = 0; y < lineCount; y++) {
        camera.ignoreHorizontalPaddingLeft();

        uint16_t x = 0;
        while (x < lineLength) {
            camera.waitForPixelClockRisingEdge(); // YUV422 grayscale byte
            camera.readPixelByte(lineBuffer[0]);
            lineBuffer[0] = formatPixelByteGrayscaleFirst(lineBuffer[0]); 

            camera.waitForPixelClockRisingEdge(); // YUV422 color byte. Ignore.
            waitForPreviousUartByteToBeSent();
            UDR0 = lineBuffer[0];
            x++;

            camera.waitForPixelClockRisingEdge(); // YUV422 grayscale byte
            camera.readPixelByte(lineBuffer[0]);
            lineBuffer[0] = formatPixelByteGrayscaleSecond(lineBuffer[0]); 

            camera.waitForPixelClockRisingEdge(); // YUV422 color byte. Ignore.
            waitForPreviousUartByteToBeSent();
            UDR0 = lineBuffer[0];
            x++;
        }

        camera.ignoreHorizontalPaddingRight();
    }
}

uint8_t formatPixelByteGrayscaleFirst(uint8_t pixelByte) {
    // For the First byte in the parity chek byte pair the last bit is always 0.
    pixelByte &= 0b11111110;
    if (pixelByte == 0) {
        // Make pixel color always slightly above 0 since zero is a command marker.
        pixelByte |= 0b00000010;
    }
    return pixelByte;
}

uint8_t formatPixelByteGrayscaleSecond(uint8_t pixelByte) {
    // For the second byte in the parity chek byte pair the last bit is always 1.
    return pixelByte | 0b00000001;
}



void processRgbFrameBuffered() {
    camera.waitForVsync();
    commandDebugPrint("Vsync");

    camera.ignoreVerticalPadding();

    for (uint16_t y = 0; y < lineCount; y++) {
        lineBufferSendByte = &lineBuffer[0];
        isLineBufferSendHighByte = true; // Line starts with High byte
        isLineBufferByteFormatted = false;

        camera.ignoreHorizontalPaddingLeft();

        for (uint16_t x = 0; x < lineBufferLength; x++) {
            camera.waitForPixelClockRisingEdge();
            camera.readPixelByte(lineBuffer[x]);
            if (isSendWhileBuffering) {
                processNextRgbPixelByteInBuffer();
            }
        };

        camera.ignoreHorizontalPaddingRight();

        // Debug info to get some feedback how mutch data was processed during line read.
        processedByteCountDuringCameraRead = lineBufferSendByte - (&lineBuffer[0]);

        // send rest of the line
        while (lineBufferSendByte < &lineBuffer[lineLength * 2]) {
            processNextRgbPixelByteInBuffer();
        }
    }
}

void processNextRgbPixelByteInBuffer() {
    // Format pixel bytes and send out in different cycles.
    // There is not enough time to do both on faster frame rates.
    if (isLineBufferByteFormatted) {
        tryToSendNextRgbPixelByteInBuffer();
    }
    else {
        formatNextRgbPixelByteInBuffer();
    }
}

void tryToSendNextRgbPixelByteInBuffer() {
    if (isUartReady()) {
        UDR0 = *lineBufferSendByte;
        lineBufferSendByte++;
        isLineBufferByteFormatted = false;
    }
}

void formatNextRgbPixelByteInBuffer() {
    if (isLineBufferSendHighByte) {
        *lineBufferSendByte = formatRgbPixelByteH(*lineBufferSendByte);
    }
    else {
        *lineBufferSendByte = formatRgbPixelByteL(*lineBufferSendByte);
    }
    isLineBufferByteFormatted = true;
    isLineBufferSendHighByte = !isLineBufferSendHighByte;
}




void processRgbFrameDirect() {
    camera.waitForVsync();
    commandDebugPrint("Vsync");

    camera.ignoreVerticalPadding();

    for (uint16_t y = 0; y < lineCount; y++) {
        camera.ignoreHorizontalPaddingLeft();

        for (uint16_t x = 0; x < lineLength; x++) {

            camera.waitForPixelClockRisingEdge();
            camera.readPixelByte(lineBuffer[0]);
            lineBuffer[0] = formatRgbPixelByteH(lineBuffer[0]);
            waitForPreviousUartByteToBeSent();
            UDR0 = lineBuffer[0];

            camera.waitForPixelClockRisingEdge();
            camera.readPixelByte(lineBuffer[0]);
            lineBuffer[0] = formatRgbPixelByteL(lineBuffer[0]);
            waitForPreviousUartByteToBeSent();
            UDR0 = lineBuffer[0];
        }

        camera.ignoreHorizontalPaddingRight();
    };
}


// RRRRRGGG
uint8_t formatRgbPixelByteH(uint8_t pixelByteH) {
    // Make sure that
    // A: pixel color always slightly above 0 since zero is end of line marker
    // B: odd number of bits for H byte under H_BYTE_PARITY_CHECK and H_BYTE_PARITY_INVERT to enable error correction
    if (pixelByteH & H_BYTE_PARITY_CHECK) {
        return pixelByteH & (~H_BYTE_PARITY_INVERT);
    }
    else {
        return pixelByteH | H_BYTE_PARITY_INVERT;
    }
}


// GGGBBBBB
uint8_t formatRgbPixelByteL(uint8_t pixelByteL) {
    // Make sure that
    // A: pixel color always slightly above 0 since zero is end of line marker
    // B: even number of bits for L byte under L_BYTE_PARITY_CHECK and L_BYTE_PARITY_INVERT to enable error correction
    if (pixelByteL & L_BYTE_PARITY_CHECK) {
        return pixelByteL | L_BYTE_PARITY_INVERT | L_BYTE_PREVENT_ZERO;
    }
    else {
        return (pixelByteL & (~L_BYTE_PARITY_INVERT)) | L_BYTE_PREVENT_ZERO;
    }
}




void commandStartNewFrame(uint8_t pixelFormat) {
    waitForPreviousUartByteToBeSent();
    UDR0 = 0x00; // New command

    waitForPreviousUartByteToBeSent();
    UDR0 = 4; // Command length

    uint8_t checksum = 0;
    checksum = sendNextCommandByte(checksum, COMMAND_NEW_FRAME);
    checksum = sendNextCommandByte(checksum, lineLength & 0xFF); // lower 8 bits of image width
    checksum = sendNextCommandByte(checksum, lineCount & 0xFF); // lower 8 bits of image height
    checksum = sendNextCommandByte(checksum,
        ((lineLength >> 8) & 0x03) // higher 2 bits of image width
        | ((lineCount >> 6) & 0x0C) // higher 2 bits of image height
        | ((pixelFormat << 4) & 0xF0));

    waitForPreviousUartByteToBeSent();
    UDR0 = checksum;
}


void commandDebugPrint(const String debugText) {
    if (debugText.length() > 0) {

        waitForPreviousUartByteToBeSent();
        UDR0 = 0x00; // New commnad

        waitForPreviousUartByteToBeSent();
        UDR0 = debugText.length() + 1; // Command length. +1 for command code.

        uint8_t checksum = 0;
        checksum = sendNextCommandByte(checksum, COMMAND_DEBUG_DATA);
        for (uint16_t i = 0; i < debugText.length(); i++) {
            checksum = sendNextCommandByte(checksum, debugText[i]);
        }

        waitForPreviousUartByteToBeSent();
        UDR0 = checksum;
    }
}


uint8_t sendNextCommandByte(uint8_t checksum, uint8_t commandByte) {
    waitForPreviousUartByteToBeSent();
    UDR0 = commandByte;
    return checksum ^ commandByte;
}




void waitForPreviousUartByteToBeSent() {
    while (!isUartReady()); //wait for byte to transmit
}


bool isUartReady() {
    return UCSR0A & (1 << UDRE0);
}



uint8_t formatPixelByteBinary(uint8_t pixelByte) { //Tislenko

    if (pixelByte <= BinaryLimit) {
        pixelByte = 1;
    }
    else {
        pixelByte = 255;
    }
    return pixelByte;
}


void setBinaryResult(uint8_t value, uint16_t nr) {

    if (value <= BinaryLimit) {
        LINE_0_FRAME[nr] = 1;
    }
    else {
        LINE_0_FRAME[nr] = 0;
    }
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
    }
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


#endif


