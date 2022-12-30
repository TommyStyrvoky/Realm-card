# Realm-card
A compact ESP32 TFT display/lipo charge circuit prop that loops through animations stored in folders on µSD card attached to PCB. Estimated battery life is ~2 hours. 

The length of the animations are limited by the resolution, as the maximum number of frames is limited by RAM space.
The animations are stored in directories under the root of the SD card, these folders are read from and 24 bit bmp images starting from and index of 1 to the maximum number of frames availible or maximum memory will be stored.


The addition of -L to the end of a directory name will make it loop without a randomized delay after completion of the animation.


See Adafruit BOM for additional components needed for the electronics aside from PCB, these were purchased through Adafruit's store, Digi-Key, and Mouser Electronics.
The ESP32 QT-PY will need to be soldered flush onto pads on the PCB, the throughholes are provided for alighment, this should be mounted flush, see assembly directory for photos. Polyamide (Kapton) tape will be used in some areas to insulate the PCB's connections on the back side, for this purpose 0.25" tape was used this is not listed on the BOM. Also not included in the BOM is some strong double sided tape, reccomend a thinner tape for adhesion of the pcb and LCD to the printed parts for removal in the future if needed to change the battery.


Included are KiCAD files for the PCB, there is a custom footprint that was created for the Adafruit QT-PY included. A step and STL file are included for the enclosure.


All production files are set up for manufacture using JLC PCB's service. BOM's and placement files are provided, missing parts may need to be substituted due to component shortages, CAD files and STL files included for printing using their SLA printers in their 8000 resin. Sanding was done after recieving the parts with some priming and sanding to reduce layer lines.
https://jlcpcb.com/


See Adafruit's ESP32 guide on setting up IDE for code upload to microcontroller.
https://learn.adafruit.com/adafruit-qt-py-esp32-pico


Adafruit libraries were used aside from the SPI library.


The enclosure was painted with Vallejo model air metallic, Mr surfacer, and Tamya paints using an airbrush and via a brush.

Primer:

mr surfacer 1000

Tamiya:

XF-1 

XF-12 

panel line accent color black

Vallejo

71.062 Aluminum

71.072 Gunmetal
