/**
 * HoverPong
 *
 * Author: Shawn Hymel @ Sparkfun Electronics
 * Date: July 8, 2015
 *
 * Play the classic game of Pong by hovering your hands above the
 * sensors.
 *
 * Hardware:
 *  - Teensy 3.1: https://www.sparkfun.com/products/12646
 *  - 32x32 LED array: https://www.sparkfun.com/products/12584
 *  - SmartMatrix: http://www.pjrc.com/store/smartmatrix_kit.html
 *  - ZX Sensors: https://www.sparkfun.com/products/12780
 *  - Power supply: http://www.adafruit.com/product/1466
 *  - Wires
 *
 * Connections:
 *  Teensy | ZX Sensors (both)
 *  -------|------------------
 *    VIN  |        VCC
 *    GND  |        GND
 *     18  |         DA
 *     19  |         CL
 *
 * Libraries:
 *  - SmartMatrix: https://github.com/pixelmatix/SmartMatrix
 *  - ZX Sensor: https://github.com/sparkfun/SparkFun_ZX_Distance_and_Gesture_Sensor_Arduino_Library
 *
 * License: This code is beerware; if you see me (or any other 
 * SparkFun employee) at the local, and you've found our code
 * helpful, please buy us a round!
 *
 * Distributed as-is; no warranty is given.
 */

#include <Wire.h>
#include <ZX_Sensor.h>
#include "SmartMatrix_32x32.h"

// Parameters
#define BRIGHTNESS          50   // Percent (up to 100%)
#define MAX_POINTS          5
#define COUNTDOWN_TIMER     3
#define DEBUG_FPS           0
#define DEBUG_GAME          0
#define PADDLE_WIDTH        4
#define SENSE_LINEARITY     1    // 0 = linear, 1 = exponential
#define FPS                 60
#define CONTROLLER_TIMEOUT  10   // ms
#define INITIAL_BALL_SPEED  0.2
#define INCREASE_SPEED      1    // 0 = constant, 1 = inc speed
#define SPEED_INC_INCREMENT 0.02

// Constants
#define ZX_ADDR_1           0x10  // I2C address for player 1
#define ZX_ADDR_2           0x11  // I2C address for player 2
#define X_MAX               241
#define X_MIDPOINT          120
#define ANALOG_IN_PIN       1     // Pin 15 is A1

// LED Matrix globals
ZX_Sensor zx_sensor_1 = ZX_Sensor(ZX_ADDR_1);
ZX_Sensor zx_sensor_2 = ZX_Sensor(ZX_ADDR_2);
SmartMatrix matrix;
const int default_brightness = BRIGHTNESS*(255/100);
const rgb24 default_background_color = {0x00, 0x00, 0x00};
const rgb24 title_color = {0xff, 0xff, 0xff};
const rgb24 playing_field_color = {0xff, 0xff, 0xff};
const rgb24 midfield_color = {0x50, 0x50, 0x50};
const rgb24 paddle_1_color = {0xff, 0xff, 0xff};
const rgb24 paddle_2_color = {0xff, 0xff, 0xff};
const rgb24 ball_color = {0xff, 0xff, 0xff};
const rgb24 p1_score_color = {0xff, 0xff, 0xff};
const rgb24 p2_score_color = {0xff, 0xff, 0xff};

// Game globals
uint8_t x_pos;
uint8_t paddle_1_y;
uint8_t paddle_2_y;
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
uint8_t p1_score = 0;
uint8_t p2_score = 0;

/***************************************************************
 * Setup
 **************************************************************/
void setup() {
  
  x_pos = 0;
  paddle_1_y = 18;
  paddle_2_y = 18;
  millis_per_frame = 1000 / FPS;
  
  // Debug
  Serial.begin(115200);
  delay(2000);
  Serial.println("HoverPong");
 
  // Seed random number generator
  randomSeed(analogRead(ANALOG_IN_PIN));
  
  // Initialize ZX Sensors
  if ( !zx_sensor_1.init() ) {
    Serial.println("Something went wrong during ZX 1 init");
  }
  
  if ( !zx_sensor_2.init() ) {
    Serial.println("Something went wrong during ZX 2 init");
  }
  
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
  playTwoPlayerGame();
}

/***************************************************************
 * Two Player Game
 ***************************************************************/

// Two player game
void playTwoPlayerGame() {
  
  uint8_t win_round;
  
  // Reset scores
  p1_score = 0;
  p2_score = 0;
  
  // Countdown to new game
  performCountdown();
  
  // Play game until someone wins
  while(1) {
  
    // Set initial parameters for game
    ball_x = 15;
    ball_y = 19;
    ball_speed = INITIAL_BALL_SPEED;
    
    // Set initial ball direction for game
    ball_theta = initBallTheta();
    
    // Play round
    win_round = playTwoPlayerRound();
#if DEBUG_GAME
    Serial.print("Round win: P");
    Serial.println(win_round);
#endif
    if ( win_round == 1) {
      p1_score++;
    } else {
      p2_score++;
    }
#if DEBUG_GAME
    Serial.print("Score: P1-");
    Serial.print(p1_score);
    Serial.print(" P2-");
    Serial.println(p2_score);
#endif
    if ( p1_score >= MAX_POINTS || p2_score >= MAX_POINTS ) {
      break;
    }
  }
  
  // Display winner
  matrix.fillScreen(default_background_color);
  if ( p1_score > p2_score ) {
    matrix.drawString(4, 6, p1_score_color, "PLAYER");
    matrix.drawChar(15, 12, p1_score_color, '1');
    matrix.drawString(7, 18, p1_score_color, "WINS!");
    matrix.swapBuffers(true);
#if DEBUG_GAME
    Serial.println("Player 1 wins!");
#endif
  } else {
    matrix.drawString(4, 6, p2_score_color, "PLAYER");
    matrix.drawChar(15, 12, p2_score_color, '2');
    matrix.drawString(7, 18, p2_score_color, "WINS!");
    matrix.swapBuffers(true);
#if DEBUG_GAME
    Serial.println("Player 2 wins!");
#endif
  }
  delay(5000);
}

// Two player round
uint8_t playTwoPlayerRound() {
  
    uint8_t deflect = 0;
  
  // Play as long as player can keep volleying
  while(1) {
    
      // Get frame start time to meet FPS requirements
    frame_start_time = millis();
    
    // Clear screen
    matrix.fillScreen(default_background_color);
    
    // Draw playing field
    drawField();
    
    // Update scores on screen
    matrix.drawChar(3, 1, p1_score_color, 0x30 + p1_score);
    matrix.drawChar(26, 1, p2_score_color, 0x30 + p2_score);
    
    // Read ZX sensor for position on player 1 and flip.
    x_pos = readXPos(zx_sensor_1);
    if ( x_pos <= 240 ) {
      x_pos = (X_MAX - 1) - x_pos;
      paddle_1_y = x_map[x_pos];
    }
    
    // Read ZX sensor for position on player 2.
    x_pos = readXPos(zx_sensor_2);
    if ( x_pos <= 240 ) {
      paddle_2_y = x_map[x_pos];
    }
    
    // Draw paddles
    matrix.drawRectangle(0, paddle_1_y, 1, \
                        (paddle_1_y + PADDLE_WIDTH - 1), \
                        paddle_1_color);
    matrix.drawRectangle(30, paddle_2_y, 31, \
                        (paddle_2_y + PADDLE_WIDTH - 1), \
                        paddle_2_color);
                        
                        
    // Update ball position
    inc_x = ball_speed * cos(ball_theta * (M_PI / 180));
    inc_y = ball_speed * sin(ball_theta * (M_PI / 180));
    ball_x += inc_x;
    ball_y += inc_y;
    
    // Check ball bounds against paddles
    if ( (ball_x < 2) && \
          (ball_y < (paddle_1_y + PADDLE_WIDTH - 1))  && \
          ((ball_y + 1) > paddle_1_y) && \
          deflect == 0) {
      diff = 2 - ball_x;
      ball_x = 2 + diff;
      ball_theta = (540 - ball_theta) % 360;
      deflect = 1;
#if DEBUG_GAME
      Serial.println("P1: Ping!");
#endif
#if INCREASE_SPEED
      ball_speed += SPEED_INC_INCREMENT;
#endif
    }
    if ( (ball_x + 1 > 29) && \
          (ball_y < (paddle_2_y + PADDLE_WIDTH - 1)) && \
          ((ball_y + 1) > paddle_2_y) && \
          deflect == 0) {
      diff = abs(29 - ball_x);
      ball_x = 29 - diff;
      ball_theta = (540 - ball_theta) % 360;
      deflect = 1;
#if DEBUG_GAME
      Serial.println("P2: Pong!");
#endif
#if INCREASE_SPEED
      ball_speed += SPEED_INC_INCREMENT;
#endif
    }
    
    // Allow ball to be deflected once it leaves paddle range
    if ( (ball_x >= 2) && (ball_x + 1 <= 29) ) {
      deflect = 0;
    }
    
    // Check ball bounds against goals
    if ( ball_x < 0 ) {
      // Score for 2
      return 2;
    }
    if ( ball_x + 1 > 31 ) {
      // Score for 1
      return 1;
    }
    
    // Check ball bounds against rails
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

// Countdown to new game
void performCountdown() {
  
  uint8_t i;
  
  for ( i = COUNTDOWN_TIMER; i > 0; i-- ) {
    matrix.fillScreen(default_background_color);
    matrix.drawString(10, 6, title_color, "NEW");
    matrix.drawString(8, 12, title_color, "GAME");
    matrix.drawChar(14, 18, title_color, 0x30 + i);
    matrix.swapBuffers(true);
    delay(1000);
  }
}

// Create a randomized ball launch angle
unsigned int initBallTheta() {
  
  unsigned int theta;
  
  theta = random(124);
  if ( theta <= 30 ) {
    theta += 15;
  } else if ( (theta > 30) && (theta <= 61) ) {
    theta += 104;
  } else if ( (theta > 61) && (theta <= 92) ) {
    theta += 133;
  } else if ( theta > 92 ) {
    theta += 222;
  }
  theta = theta % 360;
  
  return theta;
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
uint8_t readXPos(ZX_Sensor& zx_sensor) {
  
  uint8_t x_pos;
  
  // Read position from sensor over I2C
  if ( zx_sensor.positionAvailable() ) {
    x_pos = zx_sensor.readX();
    if ( x_pos != ZX_ERROR ) {
      return x_pos;
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
