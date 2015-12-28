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

//Sonar1 is left sonar and sonar2 is right sonar
NewPing sonar1(trig_pin, echo_pin, 100);
NewPing sonar2(trig2_pin, echo2_pin, 100);

//Enable pin for the left motor
int leftMotorEN = 12;

//Logic pins for the left motor
int leftMotor1 = 11;
int leftMotor2 = 10;

//Enable pin for the right motor
int rightMotorEN = 6;

//Logic pins for the right motor
int rightMotor1 = 7;
int rightMotor2 = 8;

//Used to determine whether or not we should turn left or right randomly
int leftOrRight;

//Data to be received, expecting x-coordinate, y-coordinate, auto or remote controlled
int joystick[3];
int payload = sizeof(joystick);

//Declare radio
RF24 radio(53, 48);

//Pipe for the transmitter and the receiver to use to communicate
const uint64_t pipe = 0xE8E8F0F0E1LL;

void setup() {
  pinMode(leftMotorEN, OUTPUT);
  pinMode(leftMotor1, OUTPUT);
  pinMode(leftMotor2, OUTPUT);

  pinMode(rightMotorEN, OUTPUT);
  pinMode(rightMotor1, OUTPUT);
  pinMode(rightMotor2, OUTPUT);
  
  Serial.begin(115200);
  radio.begin();
  radio.openReadingPipe(1, pipe);
  radio.startListening();

  //Seed random with an unconnected analog pin
  randomSeed(analogRead(3));
}

void loop() {
  if(radio.available()){
    while(radio.available()){
      radio.read(joystick, sizeof(joystick));
      if(joystick[2] == 1){
        //Button was pressed on the controller indicating start automatic driving 
        //and state switched to self-driving
        auto_drive();
      }else if(joystick[2] == 0){
        int x_axis = joystick[0];
        int y_axis = joystick[1];
        
        //Set initial speed to 0
        int motor1_speed = 0, motor2_speed = 0;

        //Check if y axis is less than the center coordinate to determine whether to reverse or move forward
        //Reverse both motors so tank does not turn
        if(y_axis < 520){
          go_reverse(leftMotor1, leftMotor2);
          go_reverse(rightMotor1, rightMotor2);    
        }else{
          go_forward(leftMotor1, leftMotor2);
          go_forward(rightMotor1, rightMotor2);
        }

        //If y is centered and x is not, turn in place in the direction of the analog stick
        //Use x-axis coordinate to determine the pwm speed
        if((x_axis < 506 || x_axis > 510) && 
           (y_axis > 518 && y_axis < 530)){
          if(x_axis < 506){
            x_axis = map(x_axis, 506, 0, 0, 255);
            turnLeft();
            accelerate(x_axis, x_axis);
          }else{
            x_axis = map(x_axis, 510, 1023, 0, 255);
            turnRight();
            accelerate(x_axis, x_axis);
          }
          delay(20);
        }else if(y_axis < 522 || y_axis > 526){
          //Use y as primary driver
          y_axis = map(y_axis, 0, 1023, -255, 255);
          motor1_speed = abs(y_axis);
          motor2_speed = abs(y_axis);

          //Add or substract speed depending on x value
          //Add 255 as offset
          if(x_axis < 506 || x_axis > 510){
            x_axis = map(x_axis, 0, 1023, -255, 255);
            
            if(x_axis > 0){
              motor1_speed = (motor1_speed + x_axis) + 255;
              motor2_speed = (motor2_speed - x_axis) + 255;  
            }else{
              motor1_speed = (motor1_speed - abs(x_axis)) + 255;
              motor2_speed = (motor2_speed + abs(x_axis)) + 255;
            }

            //Speed value might reach to 770 so map back to 0 -> 255
            motor1_speed = map(motor1_speed, 0, 770, 0, 255);
            motor2_speed = map(motor2_speed, 0, 770, 0, 255);
          }
          accelerate(motor1_speed, motor2_speed);
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

void auto_drive(){
  //Get measurements from each sensor with some delay in between to mitigate noise
  int distance = sonar1.ping_cm();
  delay(30);
  int distance2 = sonar2.ping_cm();
  delay(30);

  //Check distances, if both sensors return distancese that are above 20cm then keep moving forward
  //If one sensor returns a distance inside 20cm and the other is greater than 20cm then turn to the direction
  //of the sensor with greater than 20cm distance reading.
  //Else choose whether to reverse left or right randomly
  //Speeds used are constant and currently does not depend on distance
  if((distance > 20 || distance == 0) && (distance2 > 20 || distance == 0)){
    go_forward(leftMotor1, leftMotor2);
    go_forward(rightMotor1, rightMotor2);
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
    go_reverse(leftMotor1, leftMotor2);
    go_reverse(rightMotor1, rightMotor2);
    leftOrRight = random(100);
    if(leftOrRight > 50){
      accelerate(64, 255);
    }else{
      accelerate(255, 64);    
    }
    delay(2000);
  }
}

//Turn in place, move one motor forward and the other backward
void turnRight(){
  go_forward(leftMotor1, leftMotor2);
  go_reverse(rightMotor1, rightMotor2);
}

//Turn in place, move one motor forward and the other backward
void turnLeft(){
  go_forward(rightMotor1, rightMotor2);
  go_reverse(leftMotor1, leftMotor2);
}

void accelerate(int motor1_speed, int motor2_speed){
  analogWrite(leftMotorEN, motor1_speed);
  analogWrite(rightMotorEN, motor2_speed);
}

void go_forward(int logic1, int logic2){
  digitalWrite(logic1, LOW);
  digitalWrite(logic2, HIGH);
}

void go_reverse(int logic1, int logic2){
  digitalWrite(logic1, HIGH);
  digitalWrite(logic2, LOW);
}

