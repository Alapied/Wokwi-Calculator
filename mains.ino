//Import Libraries
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Keypad.h>

//Pin Def here

//LDR
#define ldr A0;

//Keypad 1
#define P1R1 22;
#define P1R2 23;
#define P1R3 24;
#define P1R4 25;

#define P1C1 26;
#define P1C2 27;
#define P1C3 28;
#define P1C4 29;

bool error = false;

const uint8_t ROWS = 4;
const uint8_t COLS = 4;
char keys1[ROWS][COLS] = {
  { '1', '2', '3', '+' },
  { '4', '5', '6', '-' },
  { '7', '8', '9', '*' },
  { '=', '0', '.', '/' }
};

uint8_t colPins1[COLS] = { P1C1, P1C2, P1C3, P1C4 }; // Pins connected to C1, C2, C3, C4
uint8_t rowPins1[ROWS] = { P1R1, P1R2, P1R3, P1R4 }; // Pins connected to R1, R2, R3, R4


//Keypad 2
#define P2R1 32;
#define P2R2 33;
#define P2R3 34;
#define P2R4 35;

#define P2C1 36;
#define P2C2 37;
#define P2C3 38;
#define P2C4 39;

char keys2[ROWS][COLS] = {
  { '1', '2', '3', '+' },
  { '4', '5', '6', '-' },
  { '7', '8', '9', '*' },
  { '=', '0', '.', '/' }
};

uint8_t colPins2[COLS] = { P2C1, P2C2, P2C3, P2C4 }; // Pins connected to C1, C2, C3, C4
uint8_t rowPins2[ROWS] = { P2R1, P2R2, P2R3, P2R4 }; // Pins connected to R1, R2, R3, R4


//constants
byte DivisionSymb[] = {
  B00000,
  B00100,
  B00000,
  B11111,
  B11111,
  B00000,
  B00100,
  B00000
};
byte SqrRootSymb[] = {
  B00000,
  B00011,
  B00010,
  B00010,
  B00010,
  B10010,
  B01010,
  B00100
};
byte MultiplySymb[] = {
  B00000,
  B00000,
  B10001,
  B01010,
  B00100,
  B01010,
  B10001,
  B00000
};
//Arrays

char[20] rawInput; //Keeps raw input as is for display
char[20] inNum; //Keeps current number being entered
string[20] inString; //Keeps numbers and operators seperate and in order


//Global Bools
bool displayupdated = false;

//Device Int
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display
Keypad keypad1 = Keypad(makeKeymap(keys1), rowPins1, colPins1, ROWS, COLS);
Keypad keypad2 = Keypad(makeKeymap(keys2), rowPins2, colPins2, ROWS, COLS);

void setup() {
  // Initalise Env
  Serial.Begin(9600);
  

  // Password Protection
  //Serial.println("Enter Password before ")
  //Enter password  
}


void loop() {
  // keypad


  //calculate
  
  if (error){
    lcd.clear();
    lcd.setCursor(cursorx,cursory);
    lcd.print(character);
    return
  }
  //display 
}

float calculate(string[20] inputArray) {
  float firstNum;
  float secNum;
  char readin;
  for(int i = 0; i < (inputArray.length()); i++) {
    if (inputArray[i] == "*") {
      //multiply
      firstNum = floatMaker(inputArray[i - 1]);
      secondNum = floatMaker(inputArray[i + 1]);
      if(error == true) { //If error is detected error should be displayed instead.
        return 0;
      }
      inputArray = rerformArray(inputArray, i, firstNum * secondNum);
    }
  }
  for(int i = 0; i < (inputArray.length()); i++) {
    if (inputArray[i] == "+") {
      //add
      firstNum = floatMaker(inputArray[i - 1]);
      secondNum = floatMaker(inputArray[i + 1]);
      if(error == true) { //If error is detected error should be displayed instead.
        return 0;
      }
      inputArray = rerformArray(inputArray, i, firstNum + secondNum);
    }
  }
  for(int i = 0; i < (inputArray.length()); i++) {
    if (inputArray[i] == "-") {
      //subtract
      firstNum = floatMaker(inputArray[i - 1]);
      secondNum = floatMaker(inputArray[i + 1]);
      if(error == true) { //If error is detected error should be displayed instead.
        return 0;
      }
      inputArray = rerformArray(inputArray, i, firstNum - secondNum);
    }
  }
  if (inputArray[1] == "") {
    return inputArray[0];
  }
  else {
    Serial.println("Something is wrong.")
  }
}

float floatMaker(string input) {
  char readin;
  for(int j = 0; j < input.length(); j++) {
    readin = input.charAt(j);
    if (isdigit(readin) == false) {
      if (readin != '.') { //Not a digit or decial place
        error = true;
        return 0;
      }
      if (input.charAt(j - 1) == '.') {
        error = true;
        return 0;
      }
    }
  }
  return input.tofloat(); //Turns input string into float and returns 
}


String[20] reformArray(String inArray[20], int index, float newValue) {
  String newArray[20];
  
  for (int i = 0; i < index - 1; i++){ //Enters all of the arrays items upto the one before the first number used
    newArray[i] = inArray[i];
  }
  newArray[index - 1] = newValue;
  for (int i = index; i < 20; i++) {
    newArray[i] = inArray[i + 2];
  }

  return newArray[];
}

//display functions
void displayitem(char character){
  lcd.setCursor(cursorx,cursory);
  lcd.print(character);
  cursorx++;
}
void printarray(char displayarray){
  for (int i = 0; i < 20;){
    cursory = 0;
    cursorx = 0;
    displayitem(displayarray[i]);
  }
}

void updatedisplayonchange(){
  if (displayupdated == false){
    printarray(rawInput[])
    displayupdated == true
  }
}


//Input keypresses

void keypads(){
  char key1 = keypad.getKey();
  //char key2 = keypad.getKey()
  if (key1 != NO_KEY) {
    Serial.println(key1);
    if (isDigit(key1)){
      //add char to input and display arrays
    } else {
      choosekey(key1);
    }
  }
}


void choosefunckey (char key){
  //has to work this way given buttons have more than one use in some cases
  switch (key)
  {
    case "=":
      //Execute calculator function
      break;
    case "+":
      //add plus to char array
      break;
    case "-":
      //add minus to char array
      break;
    case "*":
      // add mulitply to char array
      break;
    case "/":
      //Do nothing, not used
      break;
    case ".":
      //add decimal point to char array (make float)
      break;
  }
}