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
#define BRIGHTNESS          15   // Percent (up to 100%)
#define DEBUG_FPS           1
#define PADDLE_WIDTH        4
#define SENSE_LINEARITY     1    // 0 = linear, 1 = exponential
#define FPS                 60
#define CONTROLLER_TIMEOUT  10   // ms

// Constants
#define X_MAX               240
#define X_MIDPOINT          120

// LED Matrix globals
SmartMatrix matrix;
const int default_brightness = BRIGHTNESS*(255/100);
const rgb24 default_background_color = {0x00, 0x00, 0x00};
const rgb24 playing_field_color = {0xff, 0x00, 0x00};
const rgb24 midfield_color = {0x50, 0x00, 0x00};
const rgb24 paddle_1_color = {0xff, 0x00, 0x00};
const rgb24 ball_color = {0xff, 0x00, 0x00};

// Game globals
uint8_t x_pos;
uint8_t paddle_1_y;
uint8_t *x_map;
float diff;
float inc_x;
float inc_y;
float ball_x;
float ball_y;
float ball_speed;
unsigned int ball_theta;
int ball_round_x;
int ball_round_y;
unsigned long frame_start_time;
unsigned long millis_per_frame;

/***************************************************************
 * Setup
 **************************************************************/
void setup() {
  
  x_pos = 0;
  paddle_1_y = 18;
  millis_per_frame = 1000 / FPS;
  
  // Debug
  Serial.begin(115200);
  delay(2000);
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
  x_map = createXMap(10, 26, SENSE_LINEARITY);
}

/***************************************************************
 * Main Loop
 **************************************************************/
void loop() {
  
  // Play single player game
  ball_x = 15;
  ball_y = 19;
  ball_speed = 0.3;
  ball_theta = 25;
  playSinglePlayerGame();
}

/***************************************************************
 * Single Player Game
 ***************************************************************/
void playSinglePlayerGame() {
  
  // Play as long as player can keep volleying
  while(1) {
    
      // Get frame start time to meet FPS requirements
    frame_start_time = millis();
    
    // Clear screen
    matrix.fillScreen(default_background_color);
    
    // Draw playing field
    drawField();
    
    // Read ZX sensor for position
    x_pos = readXPos();
    if ( x_pos <= 240 ) {
      paddle_1_y = x_map[x_pos];
    } else {
#if DEBUG_FPS
      Serial.println("Timeout!");
#endif
    }
    
    // Draw paddle
    matrix.drawRectangle(0, paddle_1_y, 1, \
                        (paddle_1_y + PADDLE_WIDTH - 1), \
                        paddle_1_color);
                        
    // Update ball position
    inc_x = ball_speed * cos(ball_theta * (M_PI / 180));
    inc_y = ball_speed * sin(ball_theta * (M_PI / 180));
    ball_x += inc_x;
    ball_y += inc_y;
    
    // Check ball bounds
    if ( ball_x < 0 ) {
      diff = 0 - ball_x;
      ball_x = 0 + diff;
      ball_theta = (540 - ball_theta) % 360;
    }
    if ( ball_x + 1 > 31 ) {
      diff = abs(31 - ball_x);
      ball_x = 31 - diff;
      ball_theta = (540 - ball_theta) % 360;
    }
    if ( ball_y < 10 ) {
      diff = 10 - ball_y;
      ball_y = 10 + diff;
      ball_theta = 360 - ball_theta;
    }
    if ( ball_y + 1 > 29 ) {
      diff = abs(29 - ball_y);
      ball_y = 29 - diff;
      ball_theta = 360 - ball_theta;
    }
    
    // Round to nearest pixel and draw ball
    ball_round_x = roundFloat(ball_x);
    ball_round_y = roundFloat(ball_y);
    matrix.drawRectangle( ball_round_x, \
                          ball_round_y, \
                          ball_round_x + 1, \
                          ball_round_y + 1, \
                          ball_color);
    
    // Update screen
    matrix.swapBuffers(true);
    
    // Wait necessary time to achieve FPS goal
#if DEBUG_FPS
    long leftover_frame_time = (frame_start_time + \
                                 millis_per_frame) - millis();
    leftover_frame_time = (leftover_frame_time > 0) ? \
                            leftover_frame_time : 0;
    Serial.print("Leftover (ms): ");
    Serial.print(leftover_frame_time);
    Serial.print(" ");
#endif
    while( millis() < frame_start_time + millis_per_frame) {
    }
#if DEBUG_FPS
    unsigned long frame_time = millis() - frame_start_time;
    Serial.print("Frame (ms): ");
    Serial.print(frame_time);
    Serial.print(" FPS: ");
    Serial.println(1000/frame_time);
#endif
  }
}

/***************************************************************
 * Functions
 **************************************************************/

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
  unsigned long start_time;
  
  // Capture start time so we can timeout
  start_time = millis();
  
  // Flush all received Serial data. End if timeout.
  while ( Serial1.available() > 0 ) {
    Serial1.read();
    if ( millis() >= start_time + CONTROLLER_TIMEOUT ) {
      return 0xFF;
    }
  }
  
  // Wait for new data. End if timeout.
  while ( Serial1.available() <= 0 ) {
    if ( millis() >= start_time + CONTROLLER_TIMEOUT ) {
      return 0xFF;
    }
  }
  
  // Look for X position. End if timeout.
  while ( Serial1.available() > 0 ) {
    in_byte = Serial1.read();
    if ( millis() >= start_time + CONTROLLER_TIMEOUT ) {
      return 0xFF;
    }
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
  int offset;
  int i;
  
  switch ( function ) {
      
    // Exponential mapping
    case 1:
    
      // Calculate offset
      offset = out_min;
      out_min = out_min - offset;
      out_max = out_max - offset;
      
      // Find alpha (coefficient) and beta (offset)
      alpha = (log((out_max / 2) + 1) / log(2)) / X_MIDPOINT;
      beta = out_max + 1;
      
      // Generate first half of the array
      for ( i = 0; i < X_MIDPOINT; i++ ) {
        val = pow(2, (alpha * i)) - 1;
        x_map[i] = (uint8_t) roundFloat(val) + offset;
      }
      
      // Generate second half of the array
      for ( i = X_MIDPOINT; i <= X_MAX; i++ ) {
        val = (-1) * pow(2, ((-1) * alpha * (i - X_MAX))) + beta;
        x_map[i] = (uint8_t) roundFloat(val) + offset;
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
  
  // ***TEST*** Output contents of mapping
#if 0
  delay(3000);
  for (int i = 0; i <= 240; i++ ) {
    Serial.print(i);
    Serial.print(" : ");
    Serial.println(x_map[i]);
  }
#endif
      
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
