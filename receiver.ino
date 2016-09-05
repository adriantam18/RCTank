//NewPing.h an Arduino library created by Tim Eckel for interfacing with ultrasonic distance sensors
#include <NewPing.h>

#include <SPI.h>

//RF24 Fork by github user TMRh20
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>

#include <printf.h>

int trig_pin = 30;
int echo_pin = 31;

int trig2_pin = 5;
int echo2_pin = 4;

NewPing left_sonar(trig_pin, echo_pin, 100);
NewPing right_sonar(trig2_pin, echo2_pin, 100);

//Enable pin for the left motor
int left_motor_en = 12;

//Logic pins for the left motor
int left_motor_l1 = 11;
int left_motor_l2 = 10;

//Enable pin for the right motor
int right_motor_en = 6;

//Logic pins for the right motor
int right_motor_l1 = 7;
int right_motor_l2 = 8;

//Randomly generated number to determine whether to turn left or turn right
int turn_dir;

//Data to be received, expecting x-coordinate, y-coordinate, state(0 = manual, 1 = auto)
int joystick[3];
int payload = sizeof(joystick);

//Declare radio with respective chip enable and chip select pins
RF24 radio(53, 48);

//Pipe for the transmitter and the receiver to use to communicate
const uint64_t pipe = 0xE8E8F0F0E1LL;

void setup() {
  pinMode(left_motor_en, OUTPUT);
  pinMode(left_motor_l1, OUTPUT);
  pinMode(left_motor_l2, OUTPUT);

  pinMode(right_motor_en, OUTPUT);
  pinMode(right_motor_l1, OUTPUT);
  pinMode(right_motor_l2, OUTPUT);
  
  Serial.begin(115200);
  radio.begin();
  radio.openReadingPipe(1, pipe);
  radio.startListening();

  //Seed random with an unconnected analog pin
  randomSeed(analogRead(3));
}

void loop() {
  //First check if there's data to read to know whether to move or not
  if(radio.available()){
    while(radio.available()){
      radio.read(joystick, sizeof(joystick));
      if(joystick[2] == 1){
        //state = 1 means to move automatically
        autoDrive();
      }else if(joystick[2] == 0){
        int x_coord = joystick[0];
        int y_coord = joystick[1];
        int left_motor_speed = 0, right_motor_speed = 0;
        
        //Check if y axis is less than the center coordinate to determine whether to reverse or move forward
        //Reverse or forward both motors so that the tank moves properly and does not turn in place
        if(y_coord < 520){
          goReverse(left_motor_l1, left_motor_l2);
          goReverse(right_motor_l1, right_motor_l2);    
        }else{
          goForward(left_motor_l1, left_motor_l2);
          goForward(right_motor_l1, right_motor_l2);
        }

        //If y is centered and x is not, turn in place in the direction of the analog stick
        //Map x-axis coordinate to (0, 255) to determine the pwm speed
        if(y_coord > 522 && y_coord < 526){
          int motor_speed = 0;
          if(x_coord < 506){
            motor_speed = map(x_coord, 506, 0, 0, 255);
            turnLeft();
          }else if(x_coord > 510){
            motor_speed = map(x_coord, 510, 1023, 0, 255);
            turnRight();
          }

          accelerate(motor_speed, motor_speed);
          delay(20);
        }else if(y_coord <= 522 || y_coord >= 526){
          //Use y-coordinate for determining speed
          int motor_speed = abs(map(y_coord, 0, 1023, -255, 255));
          left_motor_speed = motor_speed;
          right_motor_speed = motor_speed;

          //Used to add or substract speed to a motor depending on direction of joystick
          int add_speed = abs(map(x_coord, 0, 1023, -255, 255));

          //If joystick is to the left, slow left motor down and speed right motor up
          //If to the right, do the opposite
          if(x_coord < 506){
            left_motor_speed -= add_speed;
            right_motor_speed += add_speed;
          }else if(x_coord > 510){
            left_motor_speed += add_speed;
            right_motor_speed -= add_speed;
          }

          accelerate(left_motor_speed, right_motor_speed);
          delay(20);
        }else{
          //Joystick is centered in both x and y axis so don't move
          accelerate(0, 0);
          delay(20);
        }        
      }
    }
  }else{
    //Not receving data so don't move
    accelerate(0, 0);
    delay(20);
  }
}

void autoDrive(){
  //Get measurements from each sensor with some delay in between to mitigate noise
  int distance = left_sonar.ping_cm();
  delay(30);
  int distance2 = right_sonar.ping_cm();
  delay(30);

  /*
   * Check distances, if both sensors return distancese that are above 20cm then keep moving forward.
   * If one sensor returns a distance inside 20cm and the other is greater than 20cm then turn to the direction
   * of the sensor with greater than 20cm distance reading.
   * Else choose whether to reverse left or right randomly
   * Speeds used are constant and currently does not depend on distance
   */
  if((distance > 20 || distance == 0) && (distance2 > 20 || distance == 0)){
    goForward(left_motor_l1, left_motor_l2);
    goForward(right_motor_l1, right_motor_l2);
    accelerate(255, 255);
  }else if((distance <= 20 && distance >= 1) && (distance2 > 20 || distance2 == 0)){
    turnRight();
    accelerate(255, 255);
    delay(300);
  }else if((distance2 <= 20 && distance2 >= 1) && (distance > 20 || distance == 0)){
    turnLeft();
    accelerate(255, 255);
    delay(300);    
  }else if((distance <= 20 && distance >= 1) && (distance2 <= 20 && distance2 >= 1)){
    goReverse(left_motor_l1, left_motor_l2);
    goReverse(right_motor_l1, right_motor_l2);
    turn_dir = random(100);
    if(turn_dir > 50){
      accelerate(64, 255);
    }else{
      accelerate(255, 64);    
    }
    delay(2000);
  }
}

//Turn right in place, move left motor forward and reverse right motor
void turnRight(){
  goForward(left_motor_l1, left_motor_l2);
  goReverse(right_motor_l1, right_motor_l2);
}

//Turn left in place, move right motor forward and reverse left motor
void turnLeft(){
  goForward(right_motor_l1, right_motor_l2);
  goReverse(left_motor_l1, left_motor_l2);
}

void accelerate(int left_motor_speed, int right_motor_speed){
  analogWrite(left_motor_en, left_motor_speed);
  analogWrite(right_motor_en, right_motor_speed);
}

void goForward(int logic1, int logic2){
  digitalWrite(logic1, LOW);
  digitalWrite(logic2, HIGH);
}

void goReverse(int logic1, int logic2){
  digitalWrite(logic1, HIGH);
  digitalWrite(logic2, LOW);
}

