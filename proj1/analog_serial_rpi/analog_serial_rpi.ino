// EEPROM stuff
// #include <EEPROM.h>

// struct Settings {
//   float trueVoltage;
//   uint8_t oversamples;
//   uint16_t measureRate;
// };

//Settings settings;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(38400);
}

// state variables for parsing a command
int size = 16;    // how many chars in current piece
int argc = 4;     // how many pieces
bool closed = false;

// program accepts commands, of at most 15 chars, with at most 3 arguments, 
// each with at most 15 chars as well (string termination \0).
// they are stored here (list of strings <-> "matrix" of chars)
char argv[4][16];

void loop() {
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

  // do nothing if no command to process
  if(!size && !argc) return;

  // complain if command is invalid
  if(size == -1) {
    Serial.println("ERROR: invalid command");
    return;
  }


  // print all the pieces
  // Serial.print("> ");
  // for(int i = 0; i < argc; i++){
  //   Serial.print(argv[i]);
  //   Serial.print("; ");
  // }
  // Serial.println();
  // Serial.println();


  // process any commands
  if(!strcmp(argv[0], "add")){
    // this one adds up to 3 inputs
    if(argc == 1) {
      Serial.println("ERROR: 'add' expects 1 or more arguments");
      return;
    }
    float res = 0.0;
    for(int i = 1; i < argc; i++){
      res += atof(argv[i]);
    }
    Serial.println(res, 6);
  } else if(!strcmp(argv[0], "mult")){
    // this one multiplies only 2 inputs
    if(argc != 3) {
      Serial.println("ERROR: 'mult' expects 2 parameters");
      return;
    }
    float a = atof(argv[1]);
    float b = atof(argv[2]);
    Serial.println(a*b, 6);
  } else if(!strcmp(argv[0], "help")){
    // this one helps you
    Serial.println("Available commands:");
    Serial.println("\t- add(a, ...): adds up to 3 numbers.");
    Serial.println("\t- mult(a, b): multiplies 2 numbers.");
    Serial.println("\t- help(): provides information on all the commands.");
  } else {
    // complain
    Serial.print("ERROR: command '");
    Serial.print(argv[0]);
    Serial.println("' not found");
  }
}
