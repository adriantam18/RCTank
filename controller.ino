#include <SPI.h>

//RF24 Fork by github user TMRh20
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>

#include <printf.h>

const byte joystick_x = 0;
const byte joystick_y = 1;
const byte right_button = 3;

//state whether or not the tank should be self-driving
int state = 0;
RF24 radio(9, 10);

int joystick[3];
int joystick_size = sizeof(joystick);
const uint64_t pipe = 0xE8E8F0F0E1LL;

void setup() {
  pinMode(right_button, OUTPUT);
  digitalWrite(right_button, HIGH);
  
  Serial.begin(115200);
  radio.begin();
  radio.openWritingPipe(pipe);
}

void loop() {
  if(digitalRead(right_button) == LOW){
    if(state == 0){
      state = 1;
    }else if(state == 1){
      state = 0;
    }
  }
  
  joystick[0] = analogRead(joystick_x);
  joystick[1] = analogRead(joystick_y);
  joystick[2] = state;
  radio.write(joystick, sizeof(joystick));
}
