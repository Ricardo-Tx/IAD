#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <Servo.h>

const unsigned long ULONG_MAX = (unsigned long)(-1);

// these macros print information to USB and Bluetooth serial simultaneously
#define BOTH(func, ...) {Serial.func(__VA_ARGS__);SerialBT.func(__VA_ARGS__);} 
#define PRINTP() {if(bluetooth){Serial.print("[BT] ");}else{SerialBT.print("[USB] ");}}
#define PRINTLN(...) BOTH(println, __VA_ARGS__)
#define PRINT(...) BOTH(print, __VA_ARGS__)
#define PPRINTLN(...) {PRINTP();PRINTLN(__VA_ARGS__)}
#define PPRINT(...) {PRINTP();PRINT(__VA_ARGS__)}



// ########## PIN DECLARATION ##########
#define LASER_PIN 7             // (red) digital 5V signal for the laser

#define BLUETOOTH_RX_PIN 10     // (green wire)  https://cdn.velleman.eu/downloads/25/wpi302a4v02.pdf
#define BLUETOOTH_TX_PIN 11     // (yellow wire) https://cdn.velleman.eu/downloads/25/wpi302a4v02.pdf
#define BLUETOOTH_CON_PIN 12    // (State pin)   https://cdn.velleman.eu/downloads/25/wpi302a4v02.pdf

#define TR_PIN A2               // (green)    analog 5V signal for the Top Left LDR
#define TL_PIN A1               // (blue)     analog 5V signal for the Top Right LDR
#define BR_PIN A3               // (yellow)   analog 5V signal for the Bottom Right LDR
#define BL_PIN A0               // (purple)   analog 5V signal for the Bottom Left LDR
#define BAT_PIN A5              // (white)

#define FIRE_PIN 4              // (white)
#define LON_PIN 5               // (white)    5V PWM signal for the longitude servo
#define LAT_PIN 6               // (orange)



// ########## OBJECTS AND STATE VARIABLES ##########

// Bluetooth
SoftwareSerial SerialBT(BLUETOOTH_RX_PIN, BLUETOOTH_TX_PIN); // RX | TX

// Servos
Servo servoLat;
Servo servoLon;
Servo servoFire;

// Servo limits
const int servoLatHigh = 90;
const int servoLatLow = 15;
float latAngle = 0;
float lonSpeed = 90;
const float minBatVoltage = 5.5;

// Calibration stored in EEPROM
struct Calibration {
  // calibration resistances for each of the 4 LDRs, dark and ambient light
  unsigned long TL_DARK;
  unsigned long TL_AMB;
  unsigned long TR_DARK;
  unsigned long TR_AMB;
  unsigned long BL_DARK;
  unsigned long BL_AMB;
  unsigned long BR_DARK;
  unsigned long BR_AMB;
};
Calibration calibration;

// mechanics state variables
unsigned long lastLaserMillis = 0;
unsigned long laserDelay = 0;

unsigned long lastFiredMillis = 0;
const unsigned long fireDelay = 500;

unsigned long lastMeasureMillis = 0;
const unsigned long measureDelay = 50;

unsigned long lastTime = 0;   // time difference between loop calls

float tr = 0;
float tl = 0;
float br = 0;
float bl = 0;
float at = 0;
float ab = 0;
float ar = 0;
float al = 0;
float dvert = 0;
float dvertLast = 0;
float dhor = 0;
float dhorLast = 0;

//float x, y;

// state variables for parsing a command
int size = 16;    // how many chars in current piece
int sizeRaw = 16;    // how many chars in current piece
int argc = 4;     // how many pieces (1 command + up to 3 arguments)
bool closed = false;
bool bluetooth = false;   // whether the last command was sent through bluetooth serial
bool tracking = false;



// ########## COMMANDS ##########

// program accepts commands, of at most 15 chars, with at most 3 arguments, 
// each with at most 15 chars as well (string termination \0).
char argv[4][16];
char raw[64];

// function to parse a command and the possible values it can return
#define PARSE_OK 0
#define PARSE_ERROR 1
#define PARSE_EMPTY 2
int parse_command() {
  // restart state variables
  size = 0;
  sizeRaw = 0;
  argc = 0;
  closed = false;
  bluetooth = false;

  while(true) {
    int ch = bluetooth ? SerialBT.read() : Serial.read();
    if(ch != -1){
      raw[sizeRaw++] = (char)ch;
    }

    if(ch == -1) {
      // nothing to read. break if not parsing anything (size and argc are 0)
      if(!size && !argc) {
        ch = SerialBT.read();
        if(ch == -1) break;
        bluetooth = true;
        raw[sizeRaw++] = (char)ch;
      }
      else continue;
    } else if(ch == '\n') {
      // newline \n immediately ends the parsing, error (size = -1) if parsing isn't closed
      if(!closed) size = -1;
      break;
    } else if(closed) {
      // here ch is something different than \n. since parsing is closed do error
      size = -1;
      break;
    }

    if(size == -1) continue;  // already errored, continue and wait until ch becomes \n

    if((ch >= 'a' && ch <= 'z') ||    // lowercase letters
      (ch >= 'A' && ch <= 'Z') ||     // uppercase letters
      ch == '_' ||     // underscore
      (ch >= '0' && ch <= '9' && (size || argc)) ||   // numbers, if not first character of command name
      ((ch == '.' || ch == '-') && argc))   // decimal point and minus sign if it's an argument
    { 
      // if too many characters in a command or argument, do error and continue
      if(size > 14){
        size = -1;
        continue;
      }

      // writing something
      argv[argc][size++] = (char)ch;
    } else {
      // end of writing something
      argv[argc][size] = '\0';

      if(((ch == '(' && !argc) ||   // if parsing the command and found '('
          (ch == ',' && argc > 0 && argc < 3))    // if parsing an argument (3 or less) and found ','
      && size) 
      {
        // finished parsing this part, move onto the next
        argc++;
        size = 0;
      } else {
        // closing bracket should be the last character (except for \n)
        closed = ch == ')' && argc != 0 && (argc == 1 || size);

        if(closed && size) argc++;  // argc should be incremented for all commands not of the form `abc()`
        if(!closed) size = -1;      // weird character or misplaced ')' means error
      }
    }
  }

  raw[sizeRaw] = '\0';

  if(!size && !argc) return PARSE_EMPTY;    // nothing in the serial buffer
  else if(size == -1) return PARSE_ERROR;   // something went wrong while parsing
  else return PARSE_OK;                     // parsed ok
}

// this macro prints an error message stating how many 
// arguments the function takes
#define BAD_ARG_COUNT(x)   { PPRINT("ERROR: '");    \
    PRINT(__FUNCTION__);                           \
    PRINTLN("' expects " x " arguments");          \
    return; }

void lat(){
  if(argc != 2) BAD_ARG_COUNT("1")

  int angle = strtoul(argv[1], NULL, 10);
  servoLat.write(angle);
  latAngle = (float)angle;
} 

void lon(){
  if(argc != 2) BAD_ARG_COUNT("1")

  int vel = strtoul(argv[1], NULL, 10);
  servoLon.write(vel);
  lonSpeed = vel;
} 

void fire(){
  if(argc != 2) BAD_ARG_COUNT("1")

  int val = strtoul(argv[1], NULL, 10);
  servoFire.write(val);
  lastFiredMillis = millis();
}  

void track(){
  if(argc != 2) BAD_ARG_COUNT("1")

  tracking = strtoul(argv[1], NULL, 10);
  if(!tracking) {
    servoLon.write(90);
    lonSpeed = 90;
  }
}  

void laser(){
  if(argc != 2) BAD_ARG_COUNT("1")

  unsigned long period = strtoul(argv[1], NULL, 10);

  if(period < 2) {
    digitalWrite(LASER_PIN, period);
    laserDelay = 0;
  } else {
    lastLaserMillis = millis();
    laserDelay = period >> 1;
  }
} 

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

#define LDR_GET(attr) if(!strcmp(#attr,argv[1]) || argc == 1) { PPRINT(#attr "_PIN: ");PRINTLN(ldr(attr##_PIN)); }
void ldr(){
  // this command prints the settings stored in memory
  if(argc > 2) BAD_ARG_COUNT("0 or 1")

  LDR_GET(TL)
  LDR_GET(TR)
  LDR_GET(BL)
  LDR_GET(BR)
}

float batteryVoltage() {
  float reading = 0.0;
  for(int i = 0; i < 10; i++){
    reading += analogRead(BAT_PIN);
    delay(7);
  }
  reading /= 10.0;

  float voltage = (reading+0.5)/1024.0 * 5.0;
  voltage *= (4.7+2.0)/2.0;   // undo the voltage divider
  return voltage;
}

void battery() {
  if(argc > 1) BAD_ARG_COUNT("no")

  float volt = batteryVoltage();
  PPRINT("BATTERY: ");
  PRINT(volt);
  PRINTLN(volt < minBatVoltage ? " V (too low)" : " V");
}

#define FIELD1(x) x
#define CALIB_GET(attr) if(!strcmp(#attr,argv[1]) || argc == 1) { PPRINT(#attr ": ");PRINTLN(calibration.FIELD1(attr)); }
void defget(){
  // this command prints the settings stored in memory
  if(argc > 2) BAD_ARG_COUNT("0 or 1")

  EEPROM.get(0, calibration);

  CALIB_GET(TL_DARK)
  CALIB_GET(TL_AMB)
  CALIB_GET(TR_DARK)
  CALIB_GET(TR_AMB)
  CALIB_GET(BL_DARK)
  CALIB_GET(BL_AMB)
  CALIB_GET(BR_DARK)
  CALIB_GET(BR_AMB)
}

#define CALIB_SET(attr,val) if(!strcmp(#attr,argv[1]) && !found) { calibration.FIELD1(attr) = val; found = true; }
void defput(){
  // this command writes a setting to memory
  if(argc != 3) BAD_ARG_COUNT("2")

  EEPROM.get(0, calibration);
  unsigned long val = strtoul(argv[2], NULL, 10);

  bool found = false;
  CALIB_SET(TL_DARK,val)
  CALIB_SET(TL_AMB,val)
  CALIB_SET(TR_DARK,val)
  CALIB_SET(TR_AMB,val)
  CALIB_SET(BL_DARK,val)
  CALIB_SET(BL_AMB,val)
  CALIB_SET(BR_DARK,val)
  CALIB_SET(BR_AMB,val)
  
  EEPROM.put(0, calibration);
  PPRINTLN(found ? "OK" : "NOT FOUND");
}

void help(){
  // stored in flash memory
  // REWRITE THIS
  PPRINTLN(F("Available commands:"));
  PPRINTLN(F("\t- help(): provides information on all the commands."));
  PPRINTLN(F("\t- add(a, ...): adds from 1 to 3 numbers."));
  PPRINTLN(F("\t- mult(a, b): multiplies 2 numbers."));
  PPRINTLN(F("\t- lat(angle): sets the angle of the latitude servo (0 to 180)."));
  PPRINTLN(F("\t- lon(angle): sets the speed of the longitude servo (45 to 135)."));
  PPRINTLN(F("\t- defget(...): prints the setting with the provided name (TRUE_VOLTAGE, SAMPLES, INTERVAL or BROADCAST_CHN).\n\t\t"
                  "If no name is provided, print all the settings."));
  PPRINTLN(F("\t- defput(name, val): sets the value of the setting with the provided name\n\t\t"
                  "(TRUE_VOLTAGE, SAMPLES, INTERVAL or BROADCAST_CHN)."));
  PPRINTLN(F("Available settings:"));
  PPRINTLN(F("\t- TRUE_VOLTAGE: the real voltage measured at the Arduino 5V pin."));
  PPRINTLN(F("\t- SAMPLES: number of samples to take average of, to reduce noise."));
  PPRINTLN(F("\t- INTERVAL: time in ms to wait between reading broadcasts."));
  PPRINTLN(F("\t- CHANNELS: bitmask like 0b001011 specifying multiple analog ports to read when broadcasting"));
}



// ########## MAIN EXECUTION ##########

void setup() {
  Serial.begin(9600);
  SerialBT.begin(9600);

  Serial.println("INFO: type `help()` in a serial message to get information on all the commands");
  SerialBT.println("INFO: type `help()` in a serial message to get information on all the commands");
  //PRINTLN.println("INFO: type `help()` in a serial message to get information on all the commands");

  if (batteryVoltage() < minBatVoltage) {
    Serial.print("WARNING: Vin below ");
    Serial.print(minBatVoltage);
    Serial.println(" V. Avoid running the servos on USB power.");
    SerialBT.print("WARNING: Vin below ");
    SerialBT.print(minBatVoltage);
    SerialBT.println(" V. Avoid running the servos on USB power.");
  }


  pinMode(BLUETOOTH_CON_PIN, INPUT);
  pinMode(LASER_PIN, OUTPUT);

  // servos
  servoLon.attach(LON_PIN);
  servoLon.write(90);
  lonSpeed = 90;
  servoLat.attach(LAT_PIN);
  servoLat.write(45);
  latAngle = 45;
  servoFire.attach(FIRE_PIN);
  servoFire.write(90);

  // update settings right away
  EEPROM.get(0, calibration);
}


// this macro does inverse linear interpolation on the calibrated values
#define FIELD(pos, type) pos##_##type
#define LIGHT(pos) (((float)(ldr(pos##_PIN)-calibration.FIELD(pos,AMB)))/(calibration.FIELD(pos,DARK)-calibration.FIELD(pos,AMB)))
// this macro runs a function func if its name is equal to argv[0]
#define RUN_ARG(func) if(!strcmp(argv[0],#func) && !found) { func(); found = true; }

void loop() {
  // MAIN FEATURES
  unsigned long time = millis();
  float dtime = (time-lastTime)/1000.0;
  lastTime = time;

  // laser toggle
  if(laserDelay > 0 && time - lastLaserMillis > laserDelay) {
    digitalWrite(LASER_PIN, 1-digitalRead(LASER_PIN));
    lastLaserMillis = time;
  }

  // return fire servo to origin
  if(servoFire.read() != 90 && time - lastFiredMillis > fireDelay) {
    servoFire.write(90);
  }

  // find direction of light
  if(time - lastMeasureMillis > measureDelay) {
    // tr = analogRead(TR_PIN);
    // tl = analogRead(TL_PIN);
    // br = analogRead(BR_PIN);
    // bl = analogRead(BL_PIN);
    tr = LIGHT(TR);
    tl = LIGHT(TL);
    br = LIGHT(BR);
    bl = LIGHT(BL);

    at = (tl + tr) /2;
    ab = (bl + br) /2;
    ar = (br + tr) /2;
    al = (tl + bl) /2;

    dvertLast = dvert;
    dhorLast = dhor;
    
    dvert = at - ab;
    dhor = ar - al;

    lastMeasureMillis = time;

    Serial.print("(TL, TR, BL, BR): (");
    Serial.print(tl, 4);
    Serial.print(", ");
    Serial.print(tr, 4);
    Serial.print(", ");
    Serial.print(bl, 4);
    Serial.print(", ");
    Serial.print(br, 4);
    Serial.print(")\t\tDHOR: ");
    Serial.print(dhor, 4);
    Serial.print(",\tDVERT: ");
    Serial.print(dvert, 4);
    Serial.print(",\tLON: ");
    Serial.print(servoLon.read());
    Serial.print(",\tLAT: ");
    Serial.println(servoLat.read());
  }

  
  if(tracking){ 
    // Vertical Servo
    const float latKp = -7.0;
    const float latKd = 1.1;
    latAngle += dvert*latKp + (dvert-dvertLast)/dtime*latKd;
    latAngle = constrain(latAngle, servoLatLow, servoLatHigh);
    servoLat.write(latAngle);

    // Horizontal Servo
    const float lonKp = 8.0 * sqrt(cos(latAngle / 180.0 * 3.14));   // decent profile to avoid sudden lon movements for lat close to 90º 
    const float lonKd = 0.9;
    lonSpeed = 90 + dhor*lonKp + (dhor-dhorLast)/dtime*lonKd;   // lon only moves < 87 or > 93 so we accept some steady state error for less jitter
    lonSpeed = constrain(lonSpeed, 80, 100);
    servoLon.write(lonSpeed);
  }


  // COMMAND PROCESSING
  // check if there's a new command to process
  int result = parse_command();
  if(result != PARSE_EMPTY){
    PPRINT(">>> ");
    PRINT(raw);
  }
  if(result == PARSE_ERROR) PPRINTLN("ERROR: invalid command");
  if(result != PARSE_OK) return;

  // process any commands using the RUN_ARG macro
  bool found = false;
  RUN_ARG(help)
  RUN_ARG(lat)
  RUN_ARG(lon)
  RUN_ARG(fire)
  RUN_ARG(track)
  RUN_ARG(laser)
  RUN_ARG(defget)
  RUN_ARG(defput)
  RUN_ARG(ldr)
  RUN_ARG(battery)

  // if none of the above commands matched, print fail message
  if(!found){
    PPRINT("ERROR: command '");
    PRINT(argv[0]);
    PRINTLN("' not found");
  }
}
