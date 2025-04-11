#include <Servo.h>

Servo servo;

void setup() {
  Serial.begin(9600);
  servo.attach(2);
}

void loop() {
  int val;

  // while(Serial.available() > 0) {
  //   val = Serial.parseInt();
  //   Serial.println(val);
  //   servo.write(val);
  //   delay(5);
  // }

  servo.write(90*(sin(millis()/1000.0)+1.0));
  delay(5);
}
