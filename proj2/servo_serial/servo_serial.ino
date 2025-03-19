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
  
char input[16];
int size = 0;
void loop() 
{ 
  // gets the integer number sent through serial
  size = 0;
  input[0] = '\0';  // "clears" the string
  while(true){
    int ch = Serial.read();
    if(ch == -1) continue;    // nothing found

    if(ch == '\n'){
      // expects the serial message to end with a newline and
      // stops adding to the string if one is found
      input[size] = '\0';
      break;
    } else {
      // fills the string
      input[size++] = (char)ch;
    }
  }

  // convert the input string to an integer (of type ulong), expecting a possible servo value
  int val = strtoul(input, NULL, 10);
  if(val < 45 || val > 135) return;


  Serial.print(">>> Servo speed: ");
  Serial.println(val);

  servo1.write(val);
} 