/*
 * GoBabyGo Wild Thing Car Controller
 * Written by FIRST Team 1939 (The Kuhnigits)
 * Modified by Alex Patrick and Carlos Gutierrez for the GoBabyGo Program at Mercer University
 */

#include <Servo.h>

// Mechanical Setup - Either TWO_MOTORS or SERVO_STEERING should be true, not both!
boolean TWO_MOTORS          = true;
boolean SERVO_STEERING      = false;
boolean SPEED_POTENTIOMETER = true;
boolean DISTANCE_WARNING    = true;

// Invert one or two of the motors
boolean INVERT_1 = true;
boolean INVERT_2 = false;

// Constants
int SPEED_LIMIT = 256; // Between 0-512
int DEADBAND = 150;
int RAMPING = 2;
int WARNING_DISTANCE = 14; // Distance in inches to sound piezo
int REVERSE_PULSE    = 1000; // Talon SR is 1000
int FORWARD_PULSE    = 2000; // Talon SR is 2000
int SpeedReduction;
//This array will help look for the sensor with least distance measured
int inchesArray[]= {};

// Joystick Pins
int JOYSTICK_X = 1;
int JOYSTICK_Y = 2;
// Motor Pins
int MOTOR_1    = 12;//Left Motor (Green)
int MOTOR_2    = 11;//Right Motor (Blue)
// Servo Steering Pin
int SERVO      = 3;// Make sure this pin is not being used
// Speed Potentiometer Pin
int SPEED_POT  = 0;
// Ultrasonic Input Pins
int ultrasonicSensorTotal = 4;//Number of sensors
int ULTRASONICFRONTLEFT = 5;
int ULTRASONICFRONTRIGHT = 6;
int ULTRASONICBACKRIGHT = 9;
int ULTRASONICBACKLEFT = 10;

// Buzzer Pin
int PIEZO      = 13;

// Debug Over Serial - Requires a FTDI cable
boolean DEBUG = true;

// -----Don't Mess With Anything Past This Line-----

Servo motor1;
Servo motor2;
Servo servo;

void setup() {
  pinMode(JOYSTICK_X, INPUT);
  pinMode(JOYSTICK_Y, INPUT);
  motor1.attach(MOTOR_1);
  if(TWO_MOTORS) motor2.attach(MOTOR_2);
  if(SERVO_STEERING) servo.attach(SERVO);
  if(SPEED_POTENTIOMETER) pinMode(SPEED_POT, INPUT);
  if(DISTANCE_WARNING){
    pinMode(ULTRASONICFRONTRIGHT, INPUT);
    pinMode(ULTRASONICFRONTLEFT, INPUT);
    pinMode(ULTRASONICBACKRIGHT, INPUT);
    pinMode(ULTRASONICBACKLEFT, INPUT);
    pinMode(PIEZO, OUTPUT);
  }
  if(DEBUG) Serial.begin(9600);
}

void loop() {
  //Read from the joystick
  int x = analogRead(JOYSTICK_X);
  int y = analogRead(JOYSTICK_Y);
  debug("Raw X", x);
  debug("Raw Y", y);

  //Zero values within deadband
  if(abs(512-x)<DEADBAND) x = 512;
  if(abs(512-y)<DEADBAND) y = 512;

  //Map values outside deadband to inside deadband
  if(x>512) x = map(x, 512+DEADBAND, 1023, 512, 1023);
  else if (x<512) x = map(x, 0, 512-DEADBAND, 0, 512);
  if(y>512) y = map(y, 512+DEADBAND, 1023, 512, 1023);
  else if(y<512) y = map(y, 0, 512-DEADBAND, 0, 512);

  //Establish a speed limit
  int limit = SPEED_LIMIT - SpeedReduction;
  if(SPEED_POTENTIOMETER) limit = map(analogRead(SPEED_POT), 0, 1023, 0, SPEED_LIMIT - SpeedReduction);
  debug("LIMIT", limit);

  //Map speeds to within speed limit
  x = map(x, 0, 1023, 512-limit, 512+limit);
  y = map(y, 0, 1023, 512-limit, 512+limit);
  debug("X", x);
  debug("Y", y);
  
  if(TWO_MOTORS){
    int moveValue = 0;
    if(y>512) moveValue = y-512;
    else moveValue = -(512-y);
    
    int rotateValue = 0;
    if(x>512) rotateValue = x-512;
    else rotateValue = -(512-x);

    arcadeDrive(moveValue, rotateValue);
  }
  
  if(SERVO_STEERING){
    drive(y, y);
    servo.writeMicroseconds(map(x, 0, 1024, 1000, 2000));
  }

  //Ultrasonic Code
  if(DISTANCE_WARNING){
    //Looks puts sensor readings into array
    inchesArray[0] = pulseIn(ULTRASONICFRONTRIGHT, HIGH)/144;
    inchesArray[1] = pulseIn(ULTRASONICFRONTLEFT, HIGH)/144; 
    inchesArray[2] = pulseIn(ULTRASONICBACKRIGHT, HIGH)/144;
    inchesArray[3] = pulseIn(ULTRASONICBACKLEFT, HIGH)/144;
    //Sort Array
    for(int x = 0; x < ultrasonicSensorTotal; x++)
    {
      for(int x = 0; x < ultrasonicSensorTotal; x++)
      {
        int y = x + 1;
        if(inchesArray[x]>inchesArray[y])
        {
          int temp;
          temp = inchesArray[y];
          inchesArray[y]=inchesArray[x];
          inchesArray[x]= temp;
        }
      }
    }
    //Now that array is sorted we can take the smallest distance
    int inches = inchesArray[0];
    
    debug("Inches", inches);

    if(inches<WARNING_DISTANCE){
      setPiezo(true);
      SpeedReduction = (WARNING_DISTANCE-inches)*(256/WARNING_DISTANCE);
    }
    else{
      setPiezo(false);
      SpeedReduction = 0;
    }
  }
  delay(20); //Make loop run approximately 50hz
}

void arcadeDrive(int moveValue, int rotateValue) {
  int leftMotorSpeed = 0;
  int rightMotorSpeed = 0;
  if (moveValue > 0.0) {
      if (rotateValue > 0.0) {
        leftMotorSpeed = (moveValue - rotateValue);
        rightMotorSpeed = (max(moveValue, rotateValue));
      } else {
        leftMotorSpeed = max(moveValue, -rotateValue);
        rightMotorSpeed = moveValue + rotateValue;
      }
    } else {
      if (rotateValue > 0.0) {
        leftMotorSpeed = -max(-moveValue, rotateValue);
        rightMotorSpeed = moveValue + rotateValue;
      } else {
        leftMotorSpeed = moveValue - rotateValue;
        rightMotorSpeed = -max(-moveValue, -rotateValue);
      }
    }
    drive(map(leftMotorSpeed, -512, 512, 0, 1023), map(rightMotorSpeed, -512, 512, 0, 1023));
}

int prevLeft = 500;
int prevRight = 500;

void drive(int left, int right){
  int speed1 = map(left, 0, 1023, 0, FORWARD_PULSE-REVERSE_PULSE);
  // if(speed1>prevLeft+RAMPING) speed1=speed1+RAMPING;
  // else if(speed1<prevLeft-RAMPING) speed1=speed1-RAMPING;
  // if(INVERT_1) motor1.writeMicroseconds(FORWARD_PULSE-speed1);
  // else motor1.writeMicroseconds(REVERSE_PULSE+speed1);
  // prevLeft = speed1;
  motor1.writeMicroseconds(REVERSE_PULSE+speed1);
  
  int speed2 = map(right, 0, 1023, 0, FORWARD_PULSE-REVERSE_PULSE);
  // if(speed2>prevLeft+RAMPING) speed2=speed2+RAMPING;
  // else if(speed2<prevLeft-RAMPING) speed2=speed2-RAMPING;
  // if(INVERT_2) motor2.writeMicroseconds(FORWARD_PULSE-speed2);
  // else motor2.writeMicroseconds(REVERSE_PULSE+speed2);
  // prevRight = speed2;
  motor2.writeMicroseconds(REVERSE_PULSE+speed2);
}

boolean trigger = true;
int count = 0;

void setPiezo(boolean state){
  if(state){
    if(count>=4){
      trigger = !trigger;
      count = 0;
    }else{
      if(trigger) tone(PIEZO, 1300);
      else noTone(PIEZO);
    }
    count++;
  }else{
    trigger = false;
    count = 0;
    noTone(PIEZO);
  }
}

void debug(String s, int value){
  if(DEBUG){
    Serial.print(s);
    Serial.print(": ");
    Serial.println(value);
  }
}
