#include "SPI.h"
#include "SdFat.h"  // use Adafruit branch, sd card needs to be formatted to FAT16/FAT/32 with 32kb chunks
#include "Adafruit_ILI9341.h"
#include "Adafruit_ImageReader.h"
#include "Adafruit_GFX.h"
#include "TouchScreen.h"

//pins
#define TFT_DC RX
#define TFT_CS SCL
#define SD_CS SDA
#define YM A0
#define YP A1
#define XM A3
#define XP A2
#define batteryStatus TX

//touchscreen remapping needs to be calibrated
#define TS_MINX 150
#define TS_MINY 180
#define TS_MAXX 920
#define TS_MAXY 900

#define touchscreenResistanceX 328  //measure this to calibrate

//GUI elements
#define triangleWidth 20
#define heightOffset 15
#define textSize 2

#define triangleOffset 30
#define lTriangleX 30
#define rTriangleX 210

// timing variables
float frameRate = 24;
#define minDelay 500
#define maxDelay 3000
#define touchCoolDown 3000
#define debounce 250
#define maxFramesToLoad 48  //anything beyond 12 frames of 240X320 overfills ram, this is just a placeholder for the maximum frames that could be loaded at a lower resolution
uint maxFrames = maxFramesToLoad;

// global variables
uint x,
  y;
unsigned long currentFrame, lastFrameDraw, lastTouchEvent, lastpress, delayTime, lastDelay;
unsigned long currentDirectory = 1, selectedDirectory = 1;  //directory 0 = system
#define strBufferSize 64                                    //max directory name length
#define folderListCT 256                                    // max number of directories
char fileName[strBufferSize];
char folderList[folderListCT][strBufferSize];
uint folderCount;
bool uIActiveFlag, debounceFlag, delayFlag, delayAnimation = true;
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


int32_t width = 0, height = 0;

void setup() {  //first loop
  Serial.begin(115200);
  pinMode(batteryStatus, INPUT);
  tft.begin();
  if (!SD.begin(SD_CS, SD_SCK_MHZ(SD_MHZ))) {  //check sd card
    tft.fillScreen(0);         
    tft.setTextColor(ILI9341_RED);
    tft.setTextSize(textSize);
    drawCenteredText("SD Card",120,160-12);
    drawCenteredText("not detected",120,160+12);
    while (true) {}                            //do nothing
  }
  tft.fillScreen(0);  //load screen here
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(textSize);
  drawCenteredText("Reading",120,160-12);
  drawCenteredText("card.",120,160+12);
  root.open("/");
  SD.vwd()->rewind();
  while (directory.openNext(&root, O_RDONLY)) {  //read all of the directories on the sd card and store their names to memory
    if (directory.isDir()) {
      directory.printName(&Serial);
      directory.getName(folderList[folderCount], strBufferSize);
      folderCount++;
    }
    directory.close();
  }
  char folderCountChar[10];
  ltoa(folderCount, folderCountChar, 10);
  tft.fillScreen(0);  //end load screen
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(textSize);
  drawCenteredText("Found",120,160-24);
  drawCenteredText(folderCountChar,120,160);
  drawCenteredText("animations.",120,160+24);
  currentFrame = 0;
  currentDirectory = 1;
  delay(1000);
  loadFrames();
}

void loop() {                 //main
  TSPoint p = ts.getPoint();  //touchscreen events
  if (p.z > ts.pressureThreshhold) {
    y = map(p.x, TS_MINX, TS_MAXX, tft.height(), 0);
    x = map(p.y, TS_MAXY, TS_MINY, tft.width(), 0);
    uIActiveFlag = true;
    lastTouchEvent = millis();
  }

  if ((millis() - lastTouchEvent >= touchCoolDown) && uIActiveFlag) {  //hide UI if not active and load next frame set if changed
    uIActiveFlag = false;
    if (currentDirectory != selectedDirectory) {  //only load new frames if directory changed
      currentDirectory = selectedDirectory;
      currentFrame = 0;
      loadFrames();
    }
    else{
      selectedDirectory = currentDirectory;
    }
  }

  if (millis() - lastpress >= debounce) {  //set debounce flag false after elapsed
    debounceFlag = false;
  }

  if (millis() - lastDelay >= delayTime) {  //set delay flag false after elapsed
    delayFlag = false;
  }

  if (millis() - lastFrameDraw >= (1 / frameRate) * 1000) {  //draw frame from ram every frame interval and update display
    lastFrameDraw = millis();
    playFrame();
    if (uIActiveFlag) {               //draw UI
      if (!debounceFlag) {            //debounce inputs
        if ((x > 0) && (x < (50))) {  //left button pressed
          if ((y > 0) && (y <= 50)) {
            if (p.z > ts.pressureThreshhold) {
              selectedDirectory--;
              if (selectedDirectory < 1) {
                selectedDirectory = folderCount - 1;
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
              if (selectedDirectory > folderCount - 1) {
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
      drawCenteredText( folderList[selectedDirectory], 160,heightOffset+triangleWidth/2);

      tft.setCursor(0, 150);
      tft.println(x);
      tft.setCursor(0, 165);
      tft.println(y);
      //TODO draw battery %
      //TODO calculate battery viktage
    }
  }
}

void drawCenteredText(char* text,int px,int py){
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((px-w/2), (py-h/2));
  tft.println(text);
}

void loadFrames() {  //loads frames into memory from sdcard
                     //TODO add loading screen stuff
  char directory[strBufferSize];
  strcat(directory, "/");
  strcat(directory, folderList[currentDirectory]);
  strcat(directory, "/");

  ImageReturnCode stat;
  char buf[strBufferSize + strBufferSize + 10 + 4];
  char frameChar[10];
  unsigned long framesLoaded;

  tft.fillScreen(0);
  Serial.print("Loading: ");
  Serial.print(folderList[currentDirectory]);
  Serial.println("/");
  while (true) {  //keep loading frames if they exist or until no more memory can be allocated
    ltoa(framesLoaded + 1, frameChar, 10);
    strcpy(buf, directory);
    strcat(buf, frameChar);
    strcat(buf, extension);
    Serial.print("Frame: ");
    Serial.println(frameChar);
    stat = reader.loadBMP(buf, img[framesLoaded]);
    reader.printStatus(stat);
    if (stat != 0) {  //anything other than success
      break;
    }
    tft.fillScreen(0);
    tft.setTextSize(textSize);
    drawCenteredText(folderList[currentDirectory],120,160-24);
    drawCenteredText("loaded:",120,160);
    drawCenteredText(frameChar,120,160+24);
    framesLoaded++;
  }
  if (framesLoaded >= maxFramesToLoad) {  //prevent overflow
    framesLoaded = maxFramesToLoad;
  }
  maxFrames = framesLoaded;
  //TODO set framerate and Random wait logic somehow by using the directory naming structure
}

void playFrame() {  //indexes through frame objects
  img[currentFrame].draw(tft, 0, 0);
  if (currentFrame >= maxFrames) {
    currentFrame = 0;
    if (delayAnimation) {//sets random delay at end of sequence
      delayFlag == true;
      delayTime = random(minDelay, maxDelay);
      lastDelay = millis();
    }
  }
  if (!delayFlag) {  //prevents advance to next frame if delay is in place at end of sequence
    currentFrame++;
  }
}