
#include "SPI.h"
#include <SD.h>
#include "Adafruit_ILI9341.h"
#include <Adafruit_GFX.h>
#include <stdint.h>
#include "TouchScreen.h"

//pins
#define TFT_DC 7
#define TFT_CS 5
#define SD_CS 4
#define YM A0
#define YP A1
#define XM A3
#define XP A2
#define batteryStatus A6

//touchscreen remapping needs to be calibrated
#define TS_MINX 150
#define TS_MINY 180
#define TS_MAXX 920
#define TS_MAXY 900

#define touchscreenResistanceX 328  //measure this to calibrate

TouchScreen ts = TouchScreen(XP, YP, XM, YM, touchscreenResistanceX);
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

File root;

//GUI elements
#define triangleWidth 20
#define heightOffset 15
#define textSize 2
#define textWidth 7
#define textHeight 7
#define textHeightOffset (textHeight * textSize) / 2 + heightOffset
#define triangleOffset 30
#define lTriangleX 30
#define rTriangleX 210

PROGMEM uint16_t bitmapBuffer[320][240];

// timing variables
float frameRate = 15;
#define touchCoolDown 3000
#define debounce 250

// global variables
uint x, y;
unsigned long currentFrame, lastFrameDraw, lastTouchEvent, lastpress;
unsigned long currentDirectory = 1, selectedDirectory = 1;//directory 0 = system
#define strBuffer 64
#define folderListCT 128
char fileName[strBuffer];
char folderList[folderListCT][strBuffer];
unsigned long frameCount[folderListCT];
uint folderCount;
bool uIActiveFlag;
bool nextFrameFlag;
bool debounceFlag;
char extension[]= ".bmp";
#define textTrunc 8

void setup() {
  Serial.begin(9600);
  pinMode(batteryStatus, INPUT);
  tft.begin();
  tft.fillScreen(0);
  SD.begin(SD_CS);
  root = SD.open("/");
  root.rewindDirectory();
  while (true) {
    File directory = root.openNextFile();
    if (!directory) {
      break;
    }
    if (directory.isDirectory()) {
      strcpy(folderList[folderCount], directory.name());
      frameCount[folderCount] = getfileCount(directory);
      folderCount++;
    }
    directory.close();
  }
}

void loop() {
  TSPoint p = ts.getPoint();  //touchscreen events
  if (p.z > ts.pressureThreshhold) {
    y = map(p.x, TS_MINX, TS_MAXX, tft.height(), 0);
    x = map(p.y, TS_MAXY, TS_MINY, tft.width(), 0);
    uIActiveFlag = true;
    lastTouchEvent = millis();
  }

  if ((millis() - lastTouchEvent >= touchCoolDown) && uIActiveFlag) {  //hide UI if not active
    uIActiveFlag = false;
    currentDirectory = selectedDirectory;
    currentFrame = 0;
    nextFrameFlag = false;
    //TODO reset frame counter and set selected directory as active. Get frame count of new sequence
  }
  if (millis() - lastpress >= debounce) {
    debounceFlag = false;    
  }

  if (millis() - lastFrameDraw >= (1 / frameRate) * 1000) {  //draw next frame
    //TODO read file function to bitmap array
    //tft.drawRGBBitmap(0,0,bitMap,tft.width(),tft.height())
    nextFrameFlag = true;
    tft.fillScreen(0);
    lastFrameDraw = millis();
    if (uIActiveFlag) {  //draw UI
      if (!debounceFlag) {
        if ((x > 0) && (x < (50))) {  //left button pressed
          if ((y > 0) && (y <= 50)) {
            if (p.z > ts.pressureThreshhold) {
              selectedDirectory--;
              if (selectedDirectory < 1) {
                selectedDirectory = folderCount-1;
              }
              debounceFlag = true;
              lastpress = millis();
              tft.setCursor(0, 210);
              tft.println("left");
            }
          }
        }
        if ((x > 190) && (x < (240))) {  //right button pressed
          if ((y > 0) && (y <= 50)) {
            if (p.z > ts.pressureThreshhold) {
              selectedDirectory++;
              if (selectedDirectory > folderCount-1) {
                selectedDirectory = 1;
              }
              debounceFlag = true;
              lastpress = millis();
              tft.setCursor(0, 210);
              tft.println("right");
            }
          }
        }
      }
      //draw right and left buttons
      tft.fillTriangle(lTriangleX, heightOffset, lTriangleX, heightOffset + triangleWidth, lTriangleX - triangleWidth, heightOffset + triangleWidth / 2, ILI9341_WHITE);
      tft.fillTriangle(rTriangleX, heightOffset, rTriangleX, heightOffset + triangleWidth, rTriangleX + triangleWidth, heightOffset + triangleWidth / 2, ILI9341_WHITE);

      //draw text
      tft.setTextColor(ILI9341_WHITE);
      tft.setTextSize(textSize);
      uint length = strlen(folderList[selectedDirectory]);
      if (length > textTrunc) {
        tft.setCursor(tft.width() / 2 - textTrunc / 2 * textSize * textWidth, textHeightOffset);
        char sub[textTrunc];
        strncpy(sub, folderList[selectedDirectory], textTrunc);
        tft.println(sub);
      } else {
        tft.setCursor(tft.width() / 2 - length / 2 * textSize * textWidth, textHeightOffset);
        tft.println(folderList[selectedDirectory]);
      }

      tft.setTextSize(2);
      tft.setCursor(0, 150);
      tft.println(x);
      tft.setCursor(0, 165);
      tft.println(y);
      tft.setCursor(0, 180);
      tft.println(selectedDirectory);
      tft.setCursor(0, 195);
      tft.println(frameCount[selectedDirectory]);
      //draw text
      //TODO draw battery %
      //TODO add calibration code
    }
  } else {
    if (nextFrameFlag) {
      //TODO queue next frame here to memory
      char dirBuffer[64];
      strcat(dirBuffer,folderList[currentDirectory]);
      strcat(dirBuffer,"/");

      char byteArray[4];
      byteArray[0] = (int)((currentFrame >> 24) & 0xFF) ;
      byteArray[1] = (int)((currentFrame >> 16) & 0xFF) ;
      byteArray[2] = (int)((currentFrame >> 8) & 0XFF);
      byteArray[3] = (int)((currentFrame & 0XFF));

      strcat(dirBuffer,byteArray);
      strcat(dirBuffer,extension);
      readBMP(dirBuffer);
      currentFrame++;
      nextFrameFlag = false;
    }
  }
}

unsigned long getfileCount(File dir) {
  unsigned long entryCount;
  dir.rewindDirectory();
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {
      break;
    }
    entryCount++;
    entry.close();
  }
  return entryCount;
}


#define BUFFPIXEL 20
//stuff from adafruit BMP draw example
void readBMP(char *filename) {

  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel in buffer (R+G+B per pixel)
  //uint16_t lcdbuffer[BUFFPIXEL];  // pixel out buffer (16-bit per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();
  uint8_t  lcdidx = 0;
  boolean  first = true;

  // Open requested file on SD card
  if (!SD.exists(filename)) {
    return;
  }
  else{
    bmpFile = SD.open(filename);
  }
  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed
        goodBmp = true; // Supported BMP format -- proceed!
        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;
        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }
        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if((w-1) >= tft.width())  w = tft.width();
        if((h-1) >= tft.height()) h = tft.height();

        // Set TFT address window to clipped image bounds
        //tft.setAddrWindow(x, y, x+w-1, y+h-1);

        for (row=0; row<h; row++) { // For each scanline...
          // Seek to start of scan line.  It might seem labor-
          // intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping
          // and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change
          // (avoids a lot of cluster math in SD library).
          if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if(bmpFile.position() != pos) { // Need seek?
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col=0; col<w; col++) { // For each column...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              // Push LCD buffer to the display first
              if(lcdidx > 0) {
                //tft.pushColors(lcdbuffer, lcdidx, first);
                lcdidx = 0;
                first  = false;
              }
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            //lcdbuffer[lcdidx++] = tft.color565(r,g,b);
            bitmapBuffer[row][col]=tft.color565(r,g,b);
          } // end pixel
        } // end scanline
        // Write any remaining data to LCD
        if(lcdidx > 0) {
          //tft.pushColors(lcdbuffer, lcdidx, first);
        } 
        Serial.print(F("Loaded in "));
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } // end goodBmp
    }
  }
  bmpFile.close();
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}