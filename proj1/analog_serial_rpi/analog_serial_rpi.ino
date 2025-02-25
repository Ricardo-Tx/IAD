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


// Measurement settings to persist between power cycles
struct Settings {
  float trueVoltage;        // what the reference voltage is (around 5V)
  unsigned long samples;    // how many analogRead samples to average
  unsigned long interval;   // how much to wait between readings (in ms)
};
Settings settings;


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
    Serial.print("Measure Interval: ");
    Serial.print(settings.interval);
    Serial.println(" ms");
  }
}

void defput(){
  // this command writes a setting to memory
  if(argc != 3) BAD_ARG_COUNT("2")

  EEPROM.get(0, settings);
  if(!strcmp(argv[1], "TRUE_VOLTAGE")) {
    settings.trueVoltage = atof(argv[2]);
    EEPROM.put(0, settings);
  } else if(!strcmp(argv[1], "SAMPLES")) {
    settings.samples = strtoul(argv[2], NULL, 10);
    EEPROM.put(0, settings);
  } else if(!strcmp(argv[1], "INTERVAL")) {
    settings.interval = strtoul(argv[2], NULL, 10);
    EEPROM.put(0, settings);
  } else {
    Serial.print("ERROR: 'defput' field '");
    Serial.print(argv[1]);
    Serial.println("' not found");
  }
}

void analog(){
  // this command prints a voltage read from a specific analog pin
  if(argc > 2) BAD_ARG_COUNT("0 or 1")

  EEPROM.get(0, settings);
  const int pin = A0 + (argc == 2 ? atoi(argv[1]) : 0);
  Serial.println((analogRead(pin) + 0.5) * settings.trueVoltage / 1024.0, 4);
}

void help(){
  Serial.println("Available commands:");
  Serial.println("\t- add(a, ...): adds up to 3 numbers.");
  Serial.println("\t- mult(a, b): multiplies 2 numbers.");
  Serial.println("\t- help(): provides information on all the commands.");
}


// MAIN EXECUTION

void setup() {
  Serial.begin(38400);
}

void loop() {
  // normal loop stuff here

  // ...


  // check if there's a new command to process
  int result = parse_command();

  if(result == PARSE_ERROR) Serial.println("ERROR: invalid command");
  if(result != PARSE_OK) return;

  // process any commands
  bool found = false;
  RUN_ARG(add)
  RUN_ARG(mult)
  RUN_ARG(defget)
  RUN_ARG(defput)
  RUN_ARG(help)
  RUN_ARG(analog)

  if(!found){
    Serial.print("ERROR: command '");
    Serial.print(argv[0]);
    Serial.println("' not found");
  }
}
