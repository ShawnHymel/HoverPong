HoverPong
=========

Classic Pong using a 32x32 LED matrix and two ZX Sensors from XYZ Interactive. Play pong by hovering your hands over the sensors!

You will need Teensyduino: https://www.pjrc.com/teensy/td_download.html

### Hardware

 *  Teensy 3.1: https://www.sparkfun.com/products/12646
 *  32x32 LED array: https://www.sparkfun.com/products/12584
 *  SmartMatrix: http://www.pjrc.com/store/smartmatrix_kit.html
 *  ZX Sensors: https://www.sparkfun.com/products/12780
 *  Power supply: http://www.adafruit.com/product/1466
 *  Wires
 
### Connections

| Teensy | ZX Sensors (both) |
|:------:|:-----------------:|
| VIN    | VCC               |
| GND    | GND               |
| 18     | DA                |
| 19     | CL                |

### Libraries

 * SmartMatrix: https://github.com/pixelmatix/SmartMatrix
 * ZX Sensor: https://github.com/sparkfun/SparkFun_ZX_Distance_and_Gesture_Sensor_Arduino_Library
 
### License
 
License: This code is beerware; if you see me (or any other SparkFun employee) at the local, and you've found our code helpful, please buy us a round!

Distributed as-is; no warranty is given.