#include <Servo.h>

#define PRINT_LDR(x) Serial.print(#x ": ");Serial.print(ldr(x));Serial.print(" ohm\t\t");


float ldr(const uint8_t pin) {
  float reading = 0.0;
  for(int i = 0; i < 10; i++){
    reading += analogRead(pin);
    delay(7);
  }
  reading /= 10.0;

  // V = 5 * 4.7e3 / (R + 4.7e3);
  // R = 4.7e3 * ( 5/V - 1 ) 
  float voltage = (reading+0.5)/1024.0;
  float resistance = 4.7e3 * (1.0/voltage - 1.0);
  return resistance;
}


Servo servoLat;
Servo servoLon;

void setup() {
  Serial.begin(38400);
  servoLat.attach(10);
  servoLon.attach(9);

  servoLon.write(90);
}

void loop() {
  // PRINT_LDR(A0)
  // PRINT_LDR(A1)
  // PRINT_LDR(A2)
  // PRINT_LDR(A3)
  // Serial.println();


  float light0 = ldr(A0) - 8400;
  float light1 = ldr(A1) - 9700;
  float light2 = ldr(A2) - 5700;
  float light3 = ldr(A3) - 8000;
  float x = (light0 + light1 - light2 - light3) / 4.7e3;
  float y = (light2 + light1 - light0 - light3) / 4.7e3;

  Serial.print("(");
  Serial.print(x);
  Serial.print(", ");
  Serial.print(y);
  Serial.print(")");

  int lat = (int)(y*20 + 90);
  servoLat.write(lat);

  Serial.print("\t\tLAT: ");
  Serial.println(lat);

  delay(100);
}
