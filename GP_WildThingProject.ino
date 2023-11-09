/*
 * GoBabyGo Wild Thing Car Controller
 * Written by FIRST Team 1939 (The Kuhnigits)
 * Modified by Alex Patrick and Carlos Gutierrez for the GoBabyGo Program at Mercer University
 */

#include <Servo.h>

// Mechanical Setup - Either TWO_MOTORS or SERVO_STEERING should be true, not both!
boolean TWO_MOTORS = true;
boolean SERVO_STEERING = false;
boolean SPEED_POTENTIOMETER = false;
boolean DISTANCE_WARNING = true;

// Invert one or two of the motors
boolean INVERT_1 = true;
boolean INVERT_2 = false;

// Constants
int SPEED_LIMIT = 128;  // Between 0-512
int DEADBAND = 150;
int RAMPING = 2;
int WARNING_DISTANCE = int(SPEED_LIMIT / 15);  // Distance in inches to begin slowing down
int REVERSE_PULSE = 1000;                      // Talon SR is 1000
int FORWARD_PULSE = 2000;                      // Talon SR is 2000


// Joystick Pins
int JOYSTICK_X = 15;  //Blue (A1 Pin)
int JOYSTICK_Y = 16;  //Green (A2 Pin)

// RF Controller Pins
int KILLSWITCH = 4;    // A Button: LOW- Normal   HIGH- Vehicle is stopped
int SENSORSWITCH = 3;  // B Button- Turns ultrasonic sensors off
int LIGHTSWITCH = 2;   // C Button- Turns lights on and off
int PIEZOSWITCH = 1;   // D Button- Turns piezo buzzer on and off

// Motor Pins
int MOTOR_1 = 12;  //Left Motor (Yellow)
int MOTOR_2 = 11;  //Right Motor (Brown)

// Servo Steering Pin
int SERVO = 17;  // (A3 Pin) Make sure this pin is not being used

// Speed Potentiometer Pin
int SPEED_POT = 14;  //(A0 Pin)

// Ultrasonic Input Pins
int ultrasonicSensorTotal = 6;  //Number of sensors
int ULTRASONICFRONT = 8;
int ULTRASONICFRONTLEFT = 9;
int ULTRASONICFRONTRIGHT = 10;
int ULTRASONICBACK = 7;
int ULTRASONICBACKRIGHT = 5;
int ULTRASONICBACKLEFT = 6;

// Buzzer Pin
int PIEZO = 13;

// Debug Over Serial - Requires a FTDI cable
boolean DEBUG = true;



// ---------------------------Don't Change Anything Past This Line---------------------------
Servo motor1;
Servo motor2;
Servo servo;

int x = 512;
int y = 512;
int limit = SPEED_LIMIT;
int SpeedReduction = 0;

//This array will help look for the sensor with least distance measured
int inchesArray[6] = {};  //set bounds to number of sensors!

// set RF Controller Button states
int killSwitchState = LOW;    // 0: LOW- Car operates motors // 1: HIGH- Car motors are turned off
int sensorSwitchState = LOW;  // 0: LOW- Car operates sensors // 1: HIGH- Car sensors are turned off
int lightSwitchState = LOW;   // 0: LOW- Car operates lights // 1: HIGH- Car lights are turned off
int piezoSwitchState = LOW;   // 0: LOW- Car operates piezo buzzer // 1: HIGH- Car buzzer is turned off

// Set an initial time for the steps needed to operate vehicle.
unsigned long killSwitchTimeSet = millis();     //Time set for killswitch status
unsigned long eBrakeTimeSet = millis();         //Time set for emergency brake
unsigned long sensorSwitchTimeSet = millis();   //Time set for reading ultrasonic sensor status
unsigned long buzzerSwitchTimeSet = millis();   //Time set for reading buzzer status
unsigned long sensorTimeSet = millis();         //Time set for reading ultrasonic sensors

// Time intervals for timed tasks
long eBrakeInterval = 100;      //Slow vehicle every 100 ms during kill switch activation
long sensorInterval = 500;      //Read sensors every 500 milliseconds
long buttonInterval = 2000;     // Interval used to check status of buttons every 2s
long killSwitchInterval = 500;  // Check killswitch status every 500 milliseconds

void setup() {
  pinMode(JOYSTICK_X, INPUT);
  pinMode(JOYSTICK_Y, INPUT);
  pinMode(KILLSWITCH, INPUT);
  pinMode(LIGHTSWITCH, INPUT);
  pinMode(SENSORSWITCH, INPUT);
  pinMode(PIEZOSWITCH, INPUT);
  motor1.attach(MOTOR_1);
  if (TWO_MOTORS) motor2.attach(MOTOR_2);
  if (SERVO_STEERING) servo.attach(SERVO);
  if (SPEED_POTENTIOMETER) pinMode(SPEED_POT, INPUT);
  if (DISTANCE_WARNING) {
    pinMode(ULTRASONICFRONT, INPUT);
    pinMode(ULTRASONICFRONTRIGHT, INPUT);
    pinMode(ULTRASONICFRONTLEFT, INPUT);
    pinMode(ULTRASONICBACK, INPUT);
    pinMode(ULTRASONICBACKRIGHT, INPUT);
    pinMode(ULTRASONICBACKLEFT, INPUT);
    pinMode(PIEZO, OUTPUT);
  }
  if (DEBUG) Serial.begin(9600);
}

//----------------------Program Functions----------------------
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

void drive(int left, int right) {
  int speed1 = map(left, 0, 1023, 0, FORWARD_PULSE - REVERSE_PULSE);
  // if(speed1>prevLeft+RAMPING) speed1=speed1+RAMPING;
  // else if(speed1<prevLeft-RAMPING) speed1=speed1-RAMPING;
  // if(INVERT_1) motor1.writeMicroseconds(FORWARD_PULSE-speed1);
  // else motor1.writeMicroseconds(REVERSE_PULSE+speed1);
  // prevLeft = speed1;
  motor1.writeMicroseconds(REVERSE_PULSE + speed1);

  int speed2 = map(right, 0, 1023, 0, FORWARD_PULSE - REVERSE_PULSE);
  // if(speed2>prevLeft+RAMPING) speed2=speed2+RAMPING;
  // else if(speed2<prevLeft-RAMPING) speed2=speed2-RAMPING;
  // if(INVERT_2) motor2.writeMicroseconds(FORWARD_PULSE-speed2);
  // else motor2.writeMicroseconds(REVERSE_PULSE+speed2);
  // prevRight = speed2;
  motor2.writeMicroseconds(REVERSE_PULSE + speed2);
}

boolean trigger = true;
int count = 0;

void setPiezo(boolean state) {
  if (state) {
    if (count >= 4) {
      trigger = !trigger;
      count = 0;
    } else {
      if (trigger) tone(PIEZO, 1300);
      else noTone(PIEZO);
    }
    count++;
  } else {
    trigger = false;
    count = 0;
    noTone(PIEZO);
  }
}

void debug(String s, int value) {
  if (DEBUG) {
    Serial.print(s);
    Serial.print(": ");
    Serial.println(value);
  }
}


//------------------------------------------------Main Loop------------------------------------------------
void loop() 
{
  //Read the current time for timed tasks
  unsigned long currentTime = millis();

  //Establish a speed limit
  limit = SPEED_LIMIT - SpeedReduction;
  if (SPEED_POTENTIOMETER) limit = map(analogRead(SPEED_POT), 0, 1023, 0, SPEED_LIMIT - SpeedReduction);
  //debug("LIMIT", limit);//displays speed on Serial Monitor

  //Read button status from the RF Reciever
  lightSwitchState = digitalRead(LIGHTSWITCH);
  piezoSwitchState = digitalRead(PIEZOSWITCH);

  // CHECK 1 : Killswitch status
  if (currentTime - killSwitchTimeSet > killSwitchInterval) {
    //Read Killswitch Button(A) state from the RF Reciever
    killSwitchState = digitalRead(KILLSWITCH);

    if (killSwitchState == HIGH) {
      debug("A Button Pressed: KILLSWITCH ACTIVATED", int(killSwitchState));
      //Code to slow down vehicle
      if ((SpeedReduction < int(SPEED_LIMIT*0.9)) && (currentTime - eBrakeTimeSet > eBrakeInterval)) { // Speed reduced every 100 ms
        SpeedReduction += int((double)SPEED_LIMIT / 5);
        limit = SPEED_LIMIT - SpeedReduction;
        debug("BRAKING", limit);//displays speed on Serial Monitor
        eBrakeInterval = currentTime;// Time set for next period of slowing down
      }
    } 
    else //Killswitch off >> vehicle allowed to operate.
    {
      debug("KILLSWITCH INACTIVE", int(killSwitchState));
    }
    killSwitchTimeSet = currentTime;  // Set new time to read RF reciever every 500 milliseconds
  }
  
  //Read joystick
  x = analogRead(JOYSTICK_X);
  y = analogRead(JOYSTICK_Y);
  //debug("Raw X", x);
  //debug("Raw Y", y);

  //Zero values within deadband
  if (abs(512 - x) < DEADBAND) x = 512;
  if (abs(512 - y) < DEADBAND) y = 512;

  //Map values outside deadband to inside deadband
  if (x > 512) x = map(x, 512 + DEADBAND, 1023, 512, 1023);
  else if (x < 512) x = map(x, 0, 512 - DEADBAND, 0, 512);
  if (y > 512) y = map(y, 512 + DEADBAND, 1023, 512, 1023);
  else if (y < 512) y = map(y, 0, 512 - DEADBAND, 0, 512);

  //Map speeds to within speed limit
  x = map(x, 0, 1023, 512 - limit, 512 + limit);  //x defaults to 512, increases going forward, and decreases going reverse
  y = map(y, 0, 1023, 512 - limit, 512 + limit);  //y defaults to 512, increases going left, decreases going right
  //debug("X", x);//Displays X on Serial Monitor
  //debug("Y", y);//Displays Y on Serial Monitor

  if (TWO_MOTORS) {
    int moveValue = 0;
    if (y > 512) moveValue = y - 512;
    else moveValue = -(512 - y);

    int rotateValue = 0;
    if (x > 512) rotateValue = x - 512;
    else rotateValue = -(512 - x);

    arcadeDrive(moveValue, rotateValue);
  }

  if (SERVO_STEERING) {
    drive(y, y);
    servo.writeMicroseconds(map(x, 0, 1024, 1000, 2000));
  }

  //-----------------------------------Ultrasonic Sensor Code-------------------------------------

  // CHECK 2 : Check ultrasonic sensor button status
  if (currentTime - sensorSwitchTimeSet > buttonInterval) {
    //Read Sensor Button(B) state from the RF Reciever
    sensorSwitchState = digitalRead(SENSORSWITCH);

    if (sensorSwitchState == HIGH) {
      debug("B Button Pressed: SENSORS DEACTIVATED", int(sensorSwitchState));
      //De-activate sensor readings by setting distance warning mode to false
      DISTANCE_WARNING = false;
    } 
    
    else{
      //Sensor button low >> vehicle allowed to use sensors
      DISTANCE_WARNING = true;
      debug("SENSORS ACTIVATED", int(sensorSwitchState));
      debug("SPEED LIMIT", SPEED_LIMIT);
      debug("X",x);
      debug("Y",y);
    }
    sensorSwitchTimeSet = currentTime;  // Set new time to read RF reciever every 3 seconds.
  }
  
  // Following code is run if A and B Buttons are LOW
  if (DISTANCE_WARNING == true && killSwitchState == LOW) {
    //Puts new sensor readings into array every second based on the joystick values
    if (currentTime - sensorTimeSet > sensorInterval){
      if (x > 512) {  //Forward Mode: Turns on front sensors
        inchesArray[0] = int(pulseIn(ULTRASONICFRONT, HIGH)) / 144;
        debug("FRONT inches", inchesArray[0]);  //displays FL inches on SM
        inchesArray[1] = int(pulseIn(ULTRASONICFRONTRIGHT, HIGH)) / 144;
        debug("FR inches", inchesArray[1]);  //displays FR inches on SM
        inchesArray[2] = int(pulseIn(ULTRASONICFRONTLEFT, HIGH)) / 144;
        debug("FL inches", inchesArray[2]);     //displays FL inches on SM

        inchesArray[3] = WARNING_DISTANCE * 2;  //B is OFF
        inchesArray[4] = WARNING_DISTANCE * 2;  //BR is OFF
        inchesArray[5] = WARNING_DISTANCE * 2;  //BL is OFF
      } 
      else if (x < 512){ //Reverse Mode: Turns on back sensors
        inchesArray[0] = WARNING_DISTANCE * 2;  //F is OFF
        inchesArray[1] = WARNING_DISTANCE * 2;  //FR is OFF
        inchesArray[2] = WARNING_DISTANCE * 2;  //FL is OFF

        inchesArray[3] = int(pulseIn(ULTRASONICBACK, HIGH))/144;
        debug("BACK inches", inchesArray[3]);  //displays B inches on SM
        inchesArray[4] = int(pulseIn(ULTRASONICBACKRIGHT, HIGH)) / 144;
        debug("BR inches", inchesArray[4]);     //displays BR inches on SM
        inchesArray[5] = int(pulseIn(ULTRASONICBACKLEFT, HIGH))/144;
        debug("BL inches", inchesArray[5]);  //displays BL inches on SM
      }
      //If joystick is not being used, all sensors will be turned on
      else {
        inchesArray[0] = int(pulseIn(ULTRASONICFRONT, HIGH)) / 144;
        debug("FRONT inches",inchesArray[0]);//displays F inches on SM
        inchesArray[1] = int(pulseIn(ULTRASONICFRONTRIGHT, HIGH)) / 144;
        debug("FR inches",inchesArray[1]);//displays FR inches on SM
        inchesArray[2] = int(pulseIn(ULTRASONICFRONTLEFT, HIGH)) / 144;
        debug("FL inches",inchesArray[2]);//displays FL inches on SM

        inchesArray[3] = int(pulseIn(ULTRASONICBACK, HIGH))/144;
        debug("BACK inches", inchesArray[3]);  //displays B inches on SM
        inchesArray[4] = int(pulseIn(ULTRASONICBACKRIGHT, HIGH)) / 144;
        debug("BR inches", inchesArray[4]);     //displays BR inches on SM
        inchesArray[5] = int(pulseIn(ULTRASONICBACKLEFT, HIGH))/144;
        debug("BL inches", inchesArray[5]);  //displays BL inches on SM
      }

      sensorTimeSet = currentTime; //Set new time to read sensors every second
    }

    //Sort inchesArray to find smallest distance
    for (int i = 0; i < ultrasonicSensorTotal - 1; i++) {
      for (int j = 0; j < ultrasonicSensorTotal - 1; j++) {
        int y = j + 1;
        if (inchesArray[j] > inchesArray[y]) {
          int temp = inchesArray[y];
          inchesArray[y] = inchesArray[j];
          inchesArray[j] = temp;
        }
      }
    }
    //Now that array is sorted we can take the smallest distance
    int inches = inchesArray[0];

    //debug("Min inches", inches);//displays min distance on Serial Monitor

    //Vehicle slowed down based on the smallest inches reading
    if (inches < WARNING_DISTANCE && inches >= (int((double)WARNING_DISTANCE) / 1.5)) {
      setPiezo(true);
      SpeedReduction = int((double)SPEED_LIMIT / 2.5);
    } 
    else if (inches < (int((double)WARNING_DISTANCE) / 1.5) && inches >= (int((double)WARNING_DISTANCE) / 2.0)) {
      setPiezo(true);
      SpeedReduction = int((double)SPEED_LIMIT / 2.25);
    } 
    else if (inches < (int((double)WARNING_DISTANCE) / 2.0) && inches >= (int((double)WARNING_DISTANCE) / 2.5)) {
      setPiezo(true);
      SpeedReduction = int((double)SPEED_LIMIT / 1.75);
    } 
    else if (inches < (int((double)WARNING_DISTANCE) / 2.5)) {
      setPiezo(true);
      SpeedReduction = int((double)SPEED_LIMIT / 1.25);
    } 
    else {
      setPiezo(false);
      SpeedReduction = 0;
    }
    
  }

}


