/**
 * HoverPong
 *
 * Author: Shawn Hymel @ Sparkfun Electronics
 * Date: July 12, 2014
 * License: This code is beerware; if you see me (or any other 
 * SparkFun employee) at the local, and you've found our code
 * helpful, please buy us a round!
 * Distributed as-is; no warranty is given.
 *
 * To use the SmartMatrix library and 2x UARTs, you will need to
 * modify the following lines in MatrixHardware_KitV1_32x32.h:
 *
 * Change:
 *   #define ADDX_TEENSY_PIN_0   9
 *   #define ADDX_TEENSY_PIN_1   10
 * To:
 *   #define ADDX_TEENSY_PIN_0   18
 *   #define ADDX_TEENSY_PIN_1   19
 */
 
#include "SmartMatrix_32x32.h"

// Parameters
#define PADDLE_WIDTH  4
#define FPS           30

// LED Matrix globals
SmartMatrix matrix;
const int default_brightness = 15*(255/100);  // 15% brightness
const rgb24 default_background_color = {0x00, 0x00, 0x00};
const rgb24 playing_field_color = {0xff, 0x00, 0x00};
const rgb24 midfield_color = {0x50, 0x00, 0x00};
const rgb24 paddle_1_color = {0xff, 0x00, 0x00};

// Game globals
uint8_t x_pos;
uint8_t paddle_1_y;

void setup() {
  
  x_pos = 0;
  paddle_1_y = 18;
  
  // Debug
  Serial.begin(115200);
  Serial.println("HoverPong");
  
  // Initialize ZX Sensor
  Serial1.begin(115200);
  
  // Initialize matrix
  matrix.begin();
  matrix.setBrightness(default_brightness);
  matrix.setColorCorrection(cc24);

  // Clear screen
  matrix.fillScreen(default_background_color);
  matrix.swapBuffers(true);
}

void loop() {
  
  // Clear screen
  matrix.fillScreen(default_background_color);
  
  // Draw playing field
  drawField();
  
  // Read ZX sensor for position
  x_pos = readXPos();
  if ( x_pos <= 240 ) {
    paddle_1_y = map(x_pos, 0, 240, 10, (29 - PADDLE_WIDTH + 1));
    Serial.print("Y: ");
    Serial.println(paddle_1_y, DEC);
  }
  
  // Draw paddle
  matrix.drawRectangle(0, paddle_1_y, 1, \
                      (paddle_1_y + PADDLE_WIDTH - 1), \
                      paddle_1_color);
                      
  // Update screen
  matrix.swapBuffers(true);
  
  delay(1);
}

// Draw the playing field
void drawField() {
  matrix.drawRectangle(0, 8, 31, 9, playing_field_color);
  matrix.drawRectangle(0, 30, 31, 31, playing_field_color);
  matrix.drawRectangle(15, 10, 16, 11, midfield_color);
  matrix.drawRectangle(15, 13, 16, 14, midfield_color);
  matrix.drawRectangle(15, 16, 16, 17, midfield_color);
  matrix.drawRectangle(15, 19, 16, 20, midfield_color);
  matrix.drawRectangle(15, 22, 16, 23, midfield_color);
  matrix.drawRectangle(15, 25, 16, 26, midfield_color);
  matrix.drawRectangle(15, 28, 16, 29, midfield_color);
}

// Read X position from ZX Sensor
uint8_t readXPos() {
  uint8_t in_byte;
  uint8_t is_x;
  
  // Flush all UART data, looking for X position
  while ( Serial1.available() > 0 ) {
    in_byte = Serial1.read();
    if ( in_byte == 0xFA ) {
      while ( Serial1.available() == 0 ) {
      }
        in_byte = Serial1.read();
        return in_byte;
    }
  }
  
  return 0xFF;
}
