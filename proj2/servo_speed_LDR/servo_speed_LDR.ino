// http://www.bajdi.com
// Rotating a continuous servo (tested with a SpringRC SM-S4306R)
 
// At speed signal s, with the servos being fed 6.18V, they do half a turn (pi rad) in t time:
//  s       t (ms)          reading
//  45      515.98          ok
//  54      520.96          ok
//  64      604.12          ok
//  72      845.87          ok
//  81      1765.06         ok
//  90      inf
//  99      -1971.85        ok
//  108     -900.13         ok
//  118     -612.59         ok
//  126     -544.64         ok
//  135     -532.86         ok
//
// get angular velocity w (rad/s) as a function of s


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
  pinMode(A1, INPUT);
} 

long lastTime = 0;
void loop() 
{ 
  int vel = (int)(map((float)analogRead(A0), 0.0, 1023.0, 45.0, 135.0));
  if(Serial.read() == '\n'){
    Serial.println(vel);
  }

  int light = analogRead(A1);

  long currentTime = millis();
  long delta = currentTime-lastTime;
  if(light < 400 && delta > 100){
    // Serial.print("time delta: ");
    // Serial.print(delta);
    // Serial.print(" @ speed: ");
    // Serial.println(vel);
    Serial.print(delta);
    Serial.print(",");

    lastTime = currentTime;
  }

  servo1.write(vel);
} 