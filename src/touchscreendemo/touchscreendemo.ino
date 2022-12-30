// modified version of Adafruit's Touchscreen example to use for calibration of the max and min values
//use a stylus or other tool to measure the min and max values over serial for the x and y values to remap the touchscreen to the display
#define touchscreenResistanceX 328  //measure this between x- & x+ with a multimeter to calibrate


// Touch screen library with X Y and Z (pressure) readings as well
// as oversampling to avoid 'bouncing'
// This demo code returns raw readings, public domain

#include <stdint.h>
#include "TouchScreen.h"

#define YM A0
#define YP A1
#define XM A3
#define XP A2

// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 300 ohms across the X plate
TouchScreen ts = TouchScreen(XP, YP, XM, YM, touchscreenResistanceX);

void setup(void) {
    analogReadResolution(10);
  Serial.begin(9600);
}

void loop(void) {
  // a point object holds x y and z coordinates
  TSPoint p = ts.getPoint();

  // we have some minimum pressure we consider 'valid'
  // pressure of 0 means no pressing!
  if (p.z > ts.pressureThreshhold) {
     Serial.print("X = "); Serial.print(p.x);
     Serial.print("\tY = "); Serial.print(p.y);
     Serial.print("\tPressure = "); Serial.println(p.z);
  }
}
