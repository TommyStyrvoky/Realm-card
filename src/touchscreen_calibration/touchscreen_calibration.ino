// A rudimentry calibration for the display
#include "SPI.h"
#include "Adafruit_ILI9341.h"
#include <Adafruit_GFX.h>
#include "TouchScreen.h"

//pins
#define TFT_DC 7
#define TFT_CS 5
#define YM A0
#define YP A1
#define XM A3
#define XP A2

#define sampleCount 1
#define radius 4
#define outline 2
#define testCount 4
uint screenPositions[][2] = { { 25, 25 }, { 75, 25 }, { 25, 75 }, { 75, 75 } };
uint values[4][2];
uint xmin, ymin, xmax, ymax;

#define touchscreenResistanceX 328  //measure resistance across x+ to X- this to calibrate

TouchScreen ts = TouchScreen(XP, YP, XM, YM, touchscreenResistanceX);
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

void setup() {
  tft.begin();
  delay(500);
  for (int i = 0; i < testCount; i++) {
    tft.fillScreen(0);
    uint xPos = tft.width() * screenPositions[i][0] / 100;
    uint yPos = tft.height() * screenPositions[i][1] / 100;
    tft.fillCircle(xPos, yPos, radius + outline, ILI9341_WHITE);
    tft.fillCircle(xPos, yPos, radius, ILI9341_RED);
    TSPoint p = ts.getPoint();  //touchscreen events
    unsigned long sampledX, sampledY;
    for (int s = 0; s < sampleCount; s++) {
      while (p.z < ts.pressureThreshhold) {
      p = ts.getPoint();
      }
      sampledX += p.x;
      sampledY += p.y;
      values[i][0] =p.x;// sampledX / sampleCount;
      values[i][1] =p.y;// sampledY / sampleCount;
    }
    tft.fillCircle(xPos, yPos, radius, ILI9341_GREEN);
    delay(1000);
  }
  uint xDelta1 = values[0][0] - values[1][0];
  uint xDelta2 = values[3][0] - values[2][0];
  uint yDelta1 = values[0][1] - values[1][1];
  uint yDelta2 = values[3][1] - values[2][1];
  float meanX = ((float)(xDelta1 + xDelta2))/2; //tft.width()/
  float meanY = (tft.height()/2)/(float)((yDelta1 + yDelta2))/2;//tft.height()/
  xmax = (values[0][0]+values[2][0])/2- meanX / 2;
  xmin = (values[1][0]+values[3][0])/2 + meanX / 2;
  ymin = (values[0][1]+values[3][1])/2 - meanY / 2;
  ymax = (values[1][1]+values[2][1])/2 + meanY / 2;
  xmax=meanX;
  xmin=meanY;

  tft.fillScreen(0);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);

  tft.setCursor(0, 0);
  tft.println("xmin:");
  tft.setCursor(60, 0);
  tft.println(meanX);

  tft.setCursor(0, 15);
  tft.println("xmax:");
  tft.setCursor(60, 15);
  tft.println(meanY);

  tft.setCursor(0, 30);
  tft.println("ymin:");
  tft.setCursor(60, 30);
  tft.println(ymin);

  tft.setCursor(0, 45);
  tft.println("ymax:");
  tft.setCursor(60, 45);
  tft.println(ymax);
}

void loop() {
}
