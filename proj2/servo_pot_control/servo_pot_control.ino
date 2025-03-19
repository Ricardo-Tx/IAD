// http://www.bajdi.com
// Rotating a continuous servo (tested with a SpringRC SM-S4306R)
 
#include <Servo.h> 
  
Servo servo1; 
Servo servo2;
  
void setup() 
{ 
  // servo control pins
  servo1.attach(9);
  servo2.attach(10);

  Serial.begin(38400);
  pinMode(A0, INPUT);
} 
  
void loop() 
{ 
  // converts analog readings (0 to 1023) to servo signals (45 to 135)
  // since there are ~10x more possible analog readings than servo signals (they are integers),
  // using `map()` is fine
  int vel = (int)(map((float)analogRead(A0), 0.0, 1023.0, 45.0, 135.0));
  Serial.println(vel);

  servo1.write(vel);      // goes in one direction
  servo2.write(180-vel);  // goes in opposite direction

  delay(50);
} 