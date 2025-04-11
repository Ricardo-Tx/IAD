// http://www.bajdi.com
// Rotating a continuous servo (tested with a SpringRC SM-S4306R)

// THIS CODE IS INCOMPLETE
 
#include <Servo.h> 

// analog pins of the LDRs, as seen with the LDR facing you and the latitude servo at 0 deg
#define TL_PIN A0
#define TR_PIN A1
#define BL_PIN A2
#define BR_PIN A3

// digital pin for the latitude servo homing button
#define HOMING_PIN 13

  
// longitude and latitude servos
Servo servoLon; 
Servo servoLat;
// angle variables to constantly update (integral of known servo velocity + "homing button" integration constant)  
float angleLon = 0.0;
float angleLat = 0.0;

  
// gets the current angular velocity of a given servo
float angularVelocity(Servo* servo, int sig) {
  return 
}


void setup() 
{ 
  // servo control pins
  servoLon.attach(9);
  servoLat.attach(10);

  Serial.begin(38400);
  pinMode(TL_PIN, INPUT);
  pinMode(TR_PIN, INPUT);
  pinMode(BL_PIN, INPUT);
  pinMode(BR_PIN, INPUT);
} 

float analogToLux(int analog) {
  // TO DO: find the voltage divider equation with the resistance we will eventually use
  return 0.0;
}
  
void loop() 
{ 
  float lux1 = analogToLux(analogRead(A0));
  float lux2 = analogToLux(analogRead(A1));
  float lux3 = analogToLux(analogRead(A2));
  float lux4 = analogToLux(analogRead(A3));

  // light "gradient"




  servo1.write(vel);
  servo2.write(180-vel);
  delay(20);
} 