# RCTank
 
  This RC tank currently has 2 modes. (1) Manual control and (2) Automatic driving. In manual mode, the tank is controlled wirelessly
  using a joystick shield and an RF module, both connected to an Arduino UNO. In automatic mode, the tank attempts to avoid obstacles
  using readings taken from 2 distance sensors, one on the right and one on the left. The tank is driven using two motors connected to   an H-Bridge and driven by an Arduino Mega 2560.

# Materials

  *	1x Tamiya Twin-Motor Gearbox
  *	1x Tamiya Track and Wheel Set
  *	1x Tamiya Universal Plate Set
  *	1x Arduino UNO R3
  *	1x Arduino MEGA 2560
  *	1x SparkFun Joystick Shield Kit - Used as the controller of the tank
  *	1x Texas Instruments SN754410 Quadruple Half-h Driver - Used to provide current to the motors and reverse direction of the 		motors
  *	1x 1uF electrolytic capacitor
  *	1x 22uF electrolytic capacitor
  *	1x 9v battery - Power supply for the MEGA on the tank
  *	1x 4 AA battery holder
  *	2x DC motor 130 size, 6V - To replace the motors(2.1A draw at stall) from the gearbox as these ones only draw 800mA at stall
  *	2x SunFounder Ultrasonic Module HC-SR04 Distance Sensor
  *	2x nRF24L01+ Wireless Transceiver from Addicore - Used for communication between the UNO(controller) and the MEGA(receiver)
  *	2x 100uF capacitor - Soldered directly on VCC and GND of nRF24L01 to protect the modules from the noise by the motors
  *	2x 0.1uF ceramic capacitors - Soldered directly on the motors' terminals to mitigate noise
  *	4x AA batteries - External power supply for the motors

# How It Works

  The tank is controlled by a joystick. The x-axis of the joystick is laid out such that a value of 0 means the joystick is all the  	  way to the left and that a value of 1023 means the joystick is all the way to the right. The y-axis of the joystick goes from 0 to   1023, from all the way down to all the way up respectively. The controller sends the joystick's x and y coordinates to the MEGA    	  using nRF24l01+ for communication. Specifically, it sends an array of 3 values, first element being the x-axis coordinate and    the second is the y-axis coordinate, third values is 1 or 0(value switches if right button is pressed). A third value of 1 tells the   MEGA to drive automatically; And a third value of 0 tells the MEGA that the tank is to be controlled manually. The receiving MEGA     checks to see if the y-coordinate is less than the center coordinate, if it is, then the SN754410 reverses both motor otherwise it    rotates the motors forward.

  The controller checks to see if the joystick is centered on the y-axis and if x is not centered. If x is to the right, the left       motor spins forward and the right motor goes reverse to spin to the right in place. It spins the other way if x is to the left of     the center. The speed is controlled using PWM. The PWM value is obtained by mapping either ((x-center) : 0) -> (0 : 255) if going     left and ((x-center) : 1023) -> (0 : 255) if going to the right.

  Else, y-axis is used the primary accelerator. Y-axis is mapped from (0 : 1023) -> (-255 : 255). The initial speed value of both       motors is set to the absolute value of the mapped y. Then a check is done to see if x is not centered, if it’s not then x is mapped   the same way y is mapped. It then uses this mapped x value to add or subtract to a motor’s speed depending on the direction that the   joystick is pointing. It adds 255 to the speed value as an offset before finally being mapped from (0 : 770) -> (0 : 255) then two    calls to analogWrite, one for each motor.

  The self-driving mode uses distance to determine when and where to turn. The MEGA checks if both distance sensors readings’ are       greater than a certain distance and goes forward if they are. Otherwise, it checks whether one sensor is less than a certain          distance and the other sensor is greater than the same distance value. It then turns into the direction of the sensor with greater    distance. Finally, if both readings are around the same and they are both less than a certain distance, both motor reverses and       randomly choose whether to reverse to the right or to the left.

# References

  *	SparkFun Joystick Shield Guide - https://www.sparkfun.com/tutorials/171
  *	SN754410 datasheet - http://www.ti.com/lit/ds/symlink/sn754410.pdf
  *	nRF24L01+ troubleshooting - https://arduino-info.wikispaces.com/Nrf24L01-2.4GHz-HowTo
  *	Pointer on how to map joystick position to get PWM value to write - http://www.phoenixgarage.org/show_article/128
  *	H-bridge tutorial - https://itp.nyu.edu/physcomp/labs/motors-and-transistors/dc-motor-control-using-an-h-bridge/
