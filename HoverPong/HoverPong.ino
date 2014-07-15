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
#define PADDLE_WIDTH    4
#define FPS             30
#define SENSE_LINEARITY 1    // 0 = linear, 1 = exponential

// Constants
#define X_MAX           240
#define X_MIDPOINT      120

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
uint8_t *x_map;

void setup() {
  
  x_pos = 0;
  paddle_1_y = 18;
  
  // Debug
  Serial.begin(115200);
  delay(1000);
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
  
  // Create X mapping
  x_map = createXMap(0, 16, SENSE_LINEARITY);
  
  // ***TEST*** Output contents of mapping
  delay(3000);
  for (int i = 0; i <= 240; i++ ) {
    Serial.print(i);
    Serial.print(" : ");
    Serial.println(x_map[i]);
  }
}

void loop() {
  
  // Clear screen
  matrix.fillScreen(default_background_color);
  
  // Draw playing field
  drawField();
  
  // Read ZX sensor for position
  x_pos = readXPos();
  if ( x_pos <= 240 ) {
    paddle_1_y = x_map[x_pos] + 10;
    //Serial.print("Y: ");
    //Serial.println(paddle_1_y, DEC);
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

// Creates a mapping for 0 - 240 values from the ZX Sensor
uint8_t * createXMap(int out_min, int out_max, int function) {
  
  static uint8_t x_map[X_MAX];
  float val;
  float alpha;
  float beta;
  int i;
  
  switch ( function ) {
      
    // Exponential mapping
    case 1:
      
      // Find alpha (coefficient) and beta (offset)
      alpha = (log((out_max / 2) + 1) / log(2)) / X_MIDPOINT;
      beta = out_max + 1;
      
      // Generate first half of the array
      for ( i = 0; i < X_MIDPOINT; i++ ) {
        val = pow(2, (alpha * i)) - 1;
        x_map[i] = (uint8_t) roundFloat(val);
      }
      
      // Generate second half of the array
      for ( i = X_MIDPOINT; i <= X_MAX; i++ ) {
        val = (-1) * pow(2, ((-1) * alpha * (i - X_MAX))) + beta;
        x_map[i] = (uint8_t) roundFloat(val);
      }
      break;
          
    // Linear mapping
    case 0:
    default:
      for ( i = 0; i <= X_MAX; i++ ) {
        val = mapFloat(i, 0, X_MAX, out_min, out_max);
        x_map[i] = (uint8_t) roundFloat(val);
      }
  }
      
  return x_map;
}

// Maps a float value
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
 return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Rounds a floating value to an integer
int roundFloat(float x) {
  if ( x >= 0 ) {
    return (int) (x + 0.5);
  }
  return (int) (x - 0.5);
}
