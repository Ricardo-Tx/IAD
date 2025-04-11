#include <SoftwareSerial.h> // Include SoftwareSerial library

const int ledpin = LED_BUILTIN; // set ledpin to pin 13
char BluetoothData; // variable to store data from BT module
SoftwareSerial WhaddaBT(10, 11); // RX | TX

void setup() {
  // put your setup code here, to run once:
  pinMode(ledpin, OUTPUT); // Set led pin as OUTPUT
  WhaddaBT.begin(9600); // initialize the Serial connection with the BT module at 9600 baud
  WhaddaBT.println("Bluetooth connected! Please press 0 or 1 to turn the LED off or on."); // Send a message when the BT module is connected
}

void loop() {
  // put your main code here, to run repeatedly:
  // When new data is available from the BT module:
  if(WhaddaBT.available()) {
    BluetoothData = WhaddaBT.read(); // Read incoming data
    // If incoming data is 1...
    if(BluetoothData == '1') {
      digitalWrite(ledpin, HIGH); // Turn on led
      WhaddaBT.println("LED turned ON"); // Send "LED turned ON" message to BT module
    }
    else if (BluetoothData == '0') {
      digitalWrite(ledpin, LOW); // Turn off led
      WhaddaBT.println("LED turned OFF"); // Send "LED turned OFF" message to BT module
    }
  }
  delay(100); // delay of 100 ms
}