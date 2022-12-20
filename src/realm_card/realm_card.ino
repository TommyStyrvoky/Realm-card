#include "SPI.h"
#include "Adafruit_ILI9341.h"
#include <Adafruit_GFX.h>
#include <SD.h>

//pins
#define TFT_DC 7
#define TFT_CS 5
#define SD_CS 7
#define YM A0 
#define YP A1
#define XM A2
#define XP A3
#define batteryStatus A6

//GUI elements
#define triangleWidth 20
#define heightOffset 30
#define textSize = 20
#define triangleOffset = 30
#define lTriangleX = triangleOffset
#define RTriangleX = 240-triangleOffset

// timing variables
float frameRate = 24;
uint touchCoolDown = 5000;

// global variables
uint xPos,yPos,pressure;
unsigned long currentFrame,frameCount,lastFrameDraw,lastTouchEvent;
String currentDirectory,selectedDirectory,fileName;
bool uIActiveFlag;
uint textTrunc length = 10

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

void setup() {
  pinMode(batteryStatus,INPUT);
  tft.reset();
  tft.begin();
  SD.begin(SD_CS);
}

void loop() {
  TSPoint p = ts.getPoint();//touchscreen events
  if (p.z > ts.pressureThreshhold) {
    xPos = p.x;
    yPos =p.y;
    pressure =p.z;
    uIActiveFlag = true;
  if(millis()-lastFrameDraw>=(1/frameRate)){//draw next frame
    tft.drawRGBBitmap(0,0,bitMap,tft.width(),tft.height())
    lastFrameDraw = millis();
        if ((millis()-lastTouchEvent>=touchCoolDown) && uIActiveFlag){//hide UI if not active
    uIActiveFlag = false;
    //TODO reset frame counter and set selected directory as active. Get frame count of new sequence
    lastTouchEvent = millis();
  }
    if(uIActiveFlag){//draw UI
    if((x > lTriangleX) && (x < (lTriangleX + triangleWidth))) {//left button pressed
        if ((y > heightOffset) && (y <= (heightOffset + triangleWidth))) {
          //TODO decrement current file
        }
    }
    if((x > rTriangleX) && (x < (rTriangleX + triangleWidth))) {//right button pressed
        if ((y > heightOffset) && (y <= (heightOffset + triangleWidth))) {
          //TODO increment current file
        }
    }  
    //draw right and left buttons
    tft.fillTriangle(lTriangleX, heightOffset+triangleWidth, lTriangleX, heightOffset, lTriangleX+triangleWidth, heightOffset+triangleWidth/2, ILI9341_WHITE);
    tft.fillTriangle(RTriangleX, heightOffset+triangleWidth, RTriangleX, heightOffset, rTriangleX-triangleWidth, heightOffset+triangleWidth/2, ILI9341_WHITE);

    //draw text
    tft.setTextColor(ILI9341_WHITE);    
    tft.setTextSize(textSize);
    if(selectedDirectory.length()>textTrunc){
      tft.setCursor(heightOffset,);
      tft.println(selectedDirectory.substring(0,tft.width()-textTrunc/2*textSize);
    }
    else{
      tft.setCursor(heightOffset,tft.width()-selectedDirectory.length()/2*textSize);
      tft.println(selectedDirectory);
    }
    //draw text
  //TODO draw battery %
  }
  }
}
