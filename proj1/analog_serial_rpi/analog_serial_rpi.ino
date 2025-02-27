#include <EEPROM.h>

// this macro runs a function func if its name is equal to argv[0]
#define RUN_ARG(func) if(!strcmp(argv[0],#func) && !found) { func(); found = true; }

// this macro prints an error message stating how many 
// arguments the function takes
#define BAD_ARG_COUNT(x)   { Serial.print("ERROR: '");    \
    Serial.print(__FUNCTION__);                           \
    Serial.println("' expects " x " arguments");          \
    return; }

#define PARSE_OK 0
#define PARSE_ERROR 1
#define PARSE_EMPTY 2

const unsigned long ULONG_MAX = (unsigned long)(-1);

// Measurement settings to persist between power cycles
struct Settings {
  float trueVoltage;            // what the reference voltage is (around 5V)
  unsigned long samples;        // how many analogRead samples to average
  unsigned long interval;       // time between prints in broadcast
  unsigned long channels;       // channels to broadcast
};
Settings settings;

unsigned long lastBroadcastMillis = ULONG_MAX;


// state variables for parsing a command
int size = 16;    // how many chars in current piece
int argc = 4;     // how many pieces (1 command + up to 3 arguments)
bool closed = false;

// program accepts commands, of at most 15 chars, with at most 3 arguments, 
// each with at most 15 chars as well (string termination \0).
// they are stored here (list of strings <-> "matrix" of chars)
char argv[4][16];

// function to parse a command and the possible values it can return
int parse_command() {
  // restart state variables
  size = 0;
  argc = 0;
  closed = false;
  while(true) {
    int ch = Serial.read();
    if(ch == -1) {
      // nothing to read. break if not parsing anything (size and argc are 0)
      if(!size && !argc) break;
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


  if(!size && !argc) return PARSE_EMPTY;
  else if(size == -1) return PARSE_ERROR;
  else return PARSE_OK;
}

// function that returns the voltage at a given analog pin
float voltage(int pin) {
  // delay(7) https://www.skillbank.co.uk/arduino/readanalogvolts.ino
  unsigned long sum = 0;
  for(int i = 0; i < settings.samples; i++)
  {
    sum += analogRead(pin);
    delay(7);
  }
  return (sum+0.5) * settings.trueVoltage / (settings.samples * 1024.0);
}


// COMMANDS

void add(){
  // this command adds up to 3 inputs
  if(argc == 1) BAD_ARG_COUNT("1 or more")

  float res = 0.0;
  for(int i = 1; i < argc; i++){
    res += atof(argv[i]);
  }
  Serial.println(res, 6);
}

void mult(){
  // this command multiplies only 2 inputs
  if(argc != 3) BAD_ARG_COUNT("2")

  float a = atof(argv[1]);
  float b = atof(argv[2]);
  Serial.println(a*b, 6);
} 

void err() {
  if(argc > 1) BAD_ARG_COUNT("no")

  EEPROM.get(0, settings);

  Serial.print("+- ");
  // error = trueVoltage / (1024.0 *2) / (sqrt(max(samples,4)) / 2)
  Serial.print(settings.trueVoltage/(1024.0*sqrt(max(settings.samples,4))), 8);
  Serial.println(" V");
}

void defget(){
  // this command prints the settings stored in memory
  if(argc > 2) BAD_ARG_COUNT("0 or 1")

  EEPROM.get(0, settings);
  
  if(argc == 1 || !strcmp(argv[1],"TRUE_VOLTAGE")) {
    Serial.print("True Voltage: ");
    Serial.print(settings.trueVoltage);
    Serial.println(" V");
  }
  if(argc == 1 || !strcmp(argv[1],"SAMPLES")) {
    Serial.print("Samples: ");
    Serial.println(settings.samples);
  }
  if(argc == 1 || !strcmp(argv[1],"INTERVAL")) {
    Serial.print("Interval: ");
    Serial.print(settings.interval);
    Serial.println(" ms");
  }
  if(argc == 1 || !strcmp(argv[1],"CHANNELS")) {
    Serial.print("Broadcast Channels: 0b");
    for(int i = 5; i >= 0; i--){
      Serial.print(!!(settings.channels & (1 << i)));
    }
    Serial.println();
  }
}

void defput(){
  // this command writes a setting to memory
  if(argc != 3) BAD_ARG_COUNT("2")

  EEPROM.get(0, settings);
  if(!strcmp(argv[1], "TRUE_VOLTAGE")) {
    settings.trueVoltage = atof(argv[2]);
  } else if(!strcmp(argv[1], "SAMPLES")) {
    settings.samples = strtoul(argv[2], NULL, 10);
  } else if(!strcmp(argv[1], "INTERVAL")) {
    settings.interval = strtoul(argv[2], NULL, 10);
  } else if(!strcmp(argv[1], "CHANNELS")) {
    if(argv[2][1] != 'b'){
      Serial.print("ERROR: incorrectly formatted bitmask");
      return;
    }
    settings.channels = strtoul(argv[2]+2, NULL, 10);
  } else {
    Serial.print("ERROR: 'defput' field '");
    Serial.print(argv[1]);
    Serial.println("' not found");
    return;
  }
  EEPROM.put(0, settings);
}

void analog(){
  // this command prints a voltage read from a specific analog pin
  if(argc > 2) BAD_ARG_COUNT("0 or 1")

  unsigned long currentBitmask = 0;

  EEPROM.get(0, settings);
  if(argc == 2){
    if(argv[1][0] == '0' && argv[1][1] == 'b'){
      // multichannel
      currentBitmask = strtoul(argv[1]+2, NULL, 2);
    } else {
      // single channel
      const int pin = A0 + atoi(argv[1]);
      Serial.println(voltage(pin), 4);
    }
  } else {
    if(lastBroadcastMillis < ULONG_MAX){
      // multichannel
      currentBitmask = settings.channels;
    } else {
      Serial.println("ERROR: no bitmask set to print periodically; use 'anstart'");
    }
  }

  if(currentBitmask){
    for(int i = 5; i >= 0; i--){
      if(currentBitmask & (1 << i)){
        Serial.print(voltage(A0+i), 4);
        Serial.print(",");
      }
    }
    Serial.println();
  }
}

void bstart() {
  if(argc > 1) BAD_ARG_COUNT("no")

  lastBroadcastMillis = millis();
}

void bstop() {
  if(argc > 1) BAD_ARG_COUNT("no")

  lastBroadcastMillis = ULONG_MAX;
}

void help(){
  Serial.println("Available commands:");
  Serial.println("\t- help(): provides information on all the commands.");
  Serial.println("\t- add(a, ...): adds from 1 to 3 numbers.");
  Serial.println("\t- mult(a, b): multiplies 2 numbers.");
  Serial.println("\t- err(): the error of any reading in V, according to the values of TRUE_VOLTAGE and SAMPLES.");
  Serial.println("\t- defget(...): prints the setting with the provided name (TRUE_VOLTAGE, SAMPLES, INTERVAL or BROADCAST_CHN).\n\t\t"
                  "If no name is provided, print all the settings.");
  Serial.println("\t- defput(name, val): sets the value of the setting with the provided name\n\t\t"
                  "(TRUE_VOLTAGE, SAMPLES, INTERVAL or BROADCAST_CHN).");
  Serial.println("\t- analog(...): prints the input channel voltages, the argument can be a single number\n\t\t"
                  "from 0 to 6 or a bitmask like 0b001011 specifying multiple channels (LSB is A0).\n\t\t"
                  "If no argument is provided and is broadcasting, immediately print the broadcast bitmask channels.");
  //Serial.println("\t- bset(bitmask, interval): ssaves the channels specified by the bitmask and,\n\t\t"
  //                "the interval in ms between prints, for broadcasting with 'bstart'.");
  //Serial.println("\t- bget(): prints the values previously set in 'bset', and if is currently broadcasting.");
  Serial.println("\t- bstart(): starts broadcasting with the broadcast parameters in the settings.");
  Serial.println("\t- bstop(): stops broadcasting.");
}


// MAIN EXECUTION

void setup() {
  Serial.begin(38400);
  Serial.println("INFO: type `help()` in a serial message to get information on all the commands");

  // update settings right away
  EEPROM.get(0, settings);
}

void loop() {
  // check if supposed to broadcast analog readings periodically
  if(lastBroadcastMillis < ULONG_MAX){
    unsigned long currentMillis = millis();
    if(currentMillis >= lastBroadcastMillis + settings.interval){
      analog();
      lastBroadcastMillis = currentMillis;
    }
  }

  // check if there's a new command to process
  int result = parse_command();

  if(result == PARSE_ERROR) Serial.println("ERROR: invalid command");
  if(result != PARSE_OK) return;

  // process any commands
  bool found = false;
  RUN_ARG(help)
  RUN_ARG(add)
  RUN_ARG(mult)
  RUN_ARG(err)
  RUN_ARG(defget)
  RUN_ARG(defput)
  RUN_ARG(analog)
  RUN_ARG(bstart)
  RUN_ARG(bstop)

  if(!found){
    Serial.print("ERROR: command '");
    Serial.print(argv[0]);
    Serial.println("' not found");
  }
}
