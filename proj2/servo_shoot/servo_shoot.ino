#include <Servo.h>

Servo servo1;

void setup() {
  servo1.attach(9);
  Serial.begin(38400);
}

char input[16];
int size = 0;

void loop() {
  // Read command from serial
  size = 0;
  input[0] = '\0';

  while (true) {
    int ch = Serial.read();
    if (ch == -1) continue;

    if (ch == '\n') {
      input[size] = '\0';
      break;
    } else {
      input[size++] = (char)ch;
    }
  }

  servo1.write(90);

  // Check for the "shoot" command
  if (strcmp(input, "shoot") == 0) {
    Serial.println(">>> SHOOTING!");

    // Spin the servo at full speed
    servo1.write(135);  // forward at max speed

    delay(400);  // podemos calibrar este valor

    // Stop the servo
    servo1.write(90);  // stop

    Serial.println(">>> DONE");
  }

  // Check for the "back" command
  if (strcmp(input, "back") == 0) {
    Serial.println(">>> MOVING BACK!");

    // Spin the servo at reverse speed
    servo1.write(45);  // backward at same speed (lower value for reverse)

    delay(400);  // Same time to return to initial position

    // Stop the servo
    servo1.write(90);  // stop

    Serial.println(">>> DONE");
  }
}


