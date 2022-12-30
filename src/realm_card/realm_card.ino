#include "SPI.h"
#include "SdFat.h"  // use Adafruit branch, sd card needs to be formatted to FAT16/FAT/32 with 32kb chunks
#include "Adafruit_ILI9341.h"
#include "Adafruit_ImageReader.h"
#include "Adafruit_GFX.h"
#include "TouchScreen.h"

#define loadRandom true  // selects a random animation on reboot to load

//pins
#define TFT_DC RX
#define TFT_CS SCL
#define SD_CS SDA
#define XM A0
#define XP A1
#define YM A3
#define YP A2
#define batteryStatus TX


#define touchscreenResistanceX 328  //measure this between x- & x+ with a multimeter to calibrate

//touchscreen remapping needs to be calibrated using the touchscreen demo
#define TS_MINY 190
#define TS_MINX 90
#define TS_MAXY 900
#define TS_MAXX 890

//GUI elements
#define triangleWidth 20
#define heightOffset 15
#define textSize 2

#define triangleOffset 30
#define lTriangleX 30
#define rTriangleX 210

// timing variables
#define baseFrameRate 24
float frameRate = baseFrameRate;
#define minDelay 1000
#define maxDelay 3000
#define touchCoolDown 2000
#define debounce 500
#define maxFramesToLoad 256  //anything beyond 12 frames of 240X320 overfills ram, this is just a placeholder for the maximum frames that could be loaded at a lower resolution
unsigned long framesLoaded;

// global variables
uint x, y;
unsigned long currentFrame, lastFrameDraw, lastTouchEvent, lastpress, delayTime = random(minDelay, maxDelay), lastDelay;
unsigned long currentDirectory = 1, selectedDirectory = 1;  //directory 0 = system
#define strBufferSize 64                                    //max directory name length
#define folderListCT 256                                    // max number of directories
char fileName[strBufferSize];
char folderList[folderListCT][strBufferSize];
uint folderCount;
bool uIActiveFlag, debounceFlag, delayFlag, redrawFlag, loopAnimation;
char extension[] = ".bmp";
#define SD_MHZ 25

// classes
SdFat SD;
SdFile root;
SdFile directory;
TouchScreen ts = TouchScreen(XP, YP, XM, YM, touchscreenResistanceX);
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
Adafruit_ImageReader reader(SD);
Adafruit_Image img[maxFramesToLoad];


int32_t width, height;

void setup() {  //first loop
  Serial.begin(115200);
  analogReadResolution(10);  //touch library requires 10 bit ADC
  pinMode(batteryStatus, INPUT);
  tft.begin();
  if (!SD.begin(SD_CS, SD_SCK_MHZ(SD_MHZ))) {  //check sd card
    tft.fillScreen(0);
    tft.setTextColor(ILI9341_RED);
    tft.setTextSize(textSize);
    drawCenteredText("SD Card", 120, 160 - 12);
    drawCenteredText("not detected", 120, 160 + 12);
    while (true) {}  //do nothing
  }
  tft.fillScreen(0);  //load screen here
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(textSize);
  drawCenteredText("Reading", 120, 160 - 12);
  drawCenteredText("card.", 120, 160 + 12);
  root.open("/");
  SD.vwd()->rewind();
  while (directory.openNext(&root, O_RDONLY)) {  //read all of the directories on the sd card and store their names to memory
    if (directory.isDir()) {
      directory.getName(folderList[folderCount], strBufferSize);
      folderCount++;
    }
    directory.close();
  }
  char folderCountChar[10];
  //folderCount--;
  ltoa(folderCount-1, folderCountChar, 10);
  tft.fillScreen(0);  //end load screen
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(textSize);
  drawCenteredText("Found", 120, 160 - 24);
  drawCenteredText(folderCountChar, 120, 160);
  drawCenteredText("animations.", 120, 160 + 24);
  currentFrame = 0;

  if (loadRandom) {
    currentDirectory = random(1, folderCount);
  } else {
    currentDirectory = 1;
  }

  delay(1000);
  loadFrames();
}

void loop() {                 //main
  TSPoint p = ts.getPoint();  //touchscreen events
  if (p.z > ts.pressureThreshhold) {
    y = map(p.x, TS_MAXX,TS_MINX, tft.height(), 0);//flip x and y because of orientation of display
    x = map(p.y, TS_MINY, TS_MAXY, tft.width(), 0);
    if (!uIActiveFlag) {  //draw ui after first event detectd
      redrawFlag = true;
    }
    uIActiveFlag = true;
    lastTouchEvent = millis();
  }

  if ((millis() - lastTouchEvent >= touchCoolDown) && uIActiveFlag) {  //hide UI if not active and load next frame set if changed
    uIActiveFlag = false;
    if (currentDirectory != selectedDirectory) {  //only load new frames if directory changed
      currentDirectory = selectedDirectory;
      currentFrame = 0;
      loadFrames();
    } else {
      selectedDirectory = currentDirectory;
    }
  }

  if (millis() - lastpress >= debounce) {  //set debounce flag false after elapsed
    debounceFlag = false;
  }

  if (millis() - lastFrameDraw >= (1 / frameRate) * 1000) {  //draw frame from ram every frame interval and update display
    lastFrameDraw = millis();
    if (!uIActiveFlag) {  //fill with black before rendering samller images than display size
      frameRate = baseFrameRate;
      playFrame();
    }

    if (uIActiveFlag) {               //button events
      if (!debounceFlag) {            //debounce inputs
        if ((x > 0) && (x < (50))) {  //left button pressed
          if ((y > 0) && (y <= 50)) {
            if (p.z > ts.pressureThreshhold) {
              selectedDirectory--;
              if (selectedDirectory < 1) {
                selectedDirectory = folderCount - 1;
              }
              debounceFlag = true;
              redrawFlag = true;
              lastpress = millis();
            }
          }
        }
        if ((x > 190) && (x < (240))) {  //right button pressed
          if ((y > 0) && (y <= 50)) {
            if (p.z > ts.pressureThreshhold) {
              selectedDirectory++;
              if (selectedDirectory > folderCount - 1) {
                selectedDirectory = 1;
              }
              debounceFlag = true;
              redrawFlag = true;
              lastpress = millis();
            }
          }
        }
      }
    }

    if (redrawFlag) {  //draw ui on demand
      redrawFlag = false;
      tft.fillRect(0, 0, tft.width(), heightOffset + triangleOffset, 0);
      //draw right and left buttons
      tft.fillTriangle(lTriangleX, heightOffset, lTriangleX, heightOffset + triangleWidth, lTriangleX - triangleWidth, heightOffset + triangleWidth / 2, ILI9341_WHITE);
      tft.fillTriangle(rTriangleX, heightOffset, rTriangleX, heightOffset + triangleWidth, rTriangleX + triangleWidth, heightOffset + triangleWidth / 2, ILI9341_WHITE);

      //draw text
      tft.setTextColor(ILI9341_WHITE);
      tft.setTextSize(textSize);
      drawCenteredText(folderList[selectedDirectory], 120, heightOffset + triangleWidth / 2);

      //TODO calculate battery voltage
      //TODO draw battery %
    }
  }
}

void drawCenteredText(char* text, int px, int py) {
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((px - w / 2), (py - h / 2));
  tft.println(text);
}

void loadFrames() {  //loads frames into memory from sdcard //TODO fix some bug here with going from high to low frame counts results in failure to load more than one frame
  framesLoaded = 0;
  char path[strBufferSize];
  ImageReturnCode stat;
  char buffer[strBufferSize + strBufferSize + 10 + 4];
  char frameChar[10];

  strcpy(path, folderList[currentDirectory]);
  strcat(path, "/");

  tft.fillScreen(0);
  Serial.print("Loading: ");
  Serial.print(folderList[currentDirectory]);
  Serial.println("/");
  while (true) {  //keep loading frames if they exist or until no more memory can be allocated
    ltoa(framesLoaded + 1, frameChar, 10);
    strcpy(buffer, path);

    strcat(buffer, frameChar);
    strcat(buffer, extension);
    Serial.print("Frame: ");
    Serial.println(frameChar);
    reader.bmpDimensions(buffer, &width, &height);
    stat = reader.loadBMP(buffer, img[framesLoaded]);
    reader.printStatus(stat);
    if (stat != 0) {  //anything other than success
      framesLoaded--;
      break;
    }
    tft.fillScreen(0);
    tft.setTextSize(textSize);
    drawCenteredText(folderList[currentDirectory], 120, 160 - 24);
    drawCenteredText("loaded frame", 120, 160);
    drawCenteredText("to RAM:", 120, 160+ 24);
    drawCenteredText(frameChar, 120, 160 + 48);
    framesLoaded++;
  }
  if (framesLoaded >= maxFramesToLoad) {  //prevent overflow
    framesLoaded = maxFramesToLoad;
  }
  tft.fillScreen(0);
  drawCenteredText("Done", 120, 160);
  delay(250);
  tft.fillScreen(0);

  if (folderList[currentDirectory][strlen(folderList[currentDirectory]) - 2] == '-' && folderList[currentDirectory][strlen(folderList[currentDirectory]) - 1] == 'L') {  // if the directory contains -L afterwards it will ignore the delay
    loopAnimation = true;
  } else {
    loopAnimation = false;
  }
}

void playFrame() {                                                                                                                                     //indexes through frame objects
  img[currentFrame].draw(tft, constrain(tft.width() / 2 - (width / 2), 0, tft.width()), constrain(tft.height() / 2 - (height / 2), 0, tft.height()));  //draw in center of display
  if (currentFrame >= framesLoaded && !loopAnimation && !delayFlag) {
    delayFlag = true;
    delayTime = random(minDelay, maxDelay);
    lastDelay = millis();
  }
  if (millis() - lastDelay >= delayTime && delayFlag) {
    delayFlag = false;
  }
  if (!delayFlag) {  //prevents advance to next frame if delay is in place at end of sequence
    currentFrame++;
    if (currentFrame > framesLoaded) {
      currentFrame = 0;
    }
  }
}