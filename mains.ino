//Import Libraries
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Keypad.h>
#include <math.h>

//Pin Def here

//LDR
#define ldr A0

//Keypad 1
#define P1R1 22
#define P1R2 23
#define P1R3 24
#define P1R4 25
#define P1C1 26
#define P1C2 27
#define P1C3 28
#define P1C4 29

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
#define P2R1 32
#define P2R2 33
#define P2R3 34
#define P2R4 35

#define P2C1 36
#define P2C2 37
#define P2C3 38
#define P2C4 39

char keys2[ROWS][COLS] = {
  { 'T', 'CE', '<', '(' },
  { 'R', 'MC', 'MR', ')' },
  { '%', 'M-', 'M+', '_' },
  { 'O', 'C', 'F', '!' }
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

char rawInput[20]; //Keeps raw input as is for display
char inNum[20]; //Keeps current number being entered
String inString[20]; //Keeps numbers and operators seperate and in order
String inStringCopy[20]; //A copy of the inString to allow for destructive editing

int masterptr = 0;
int inNumPos = 0;
int inStringPos = 0;
int cursorx = 0;
int cursory = 0;

//Global Bools
bool displayupdated = false;
bool calc = false;
bool on = true;
bool error = false;
bool errorShown = false;

//Device Int
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display
Keypad keypad1 = Keypad(makeKeymap(keys1), rowPins1, colPins1, ROWS, COLS);
Keypad keypad2 = Keypad(makeKeymap(keys2), rowPins2, colPins2, ROWS, COLS);

void setup() {
  // Initalise Env
  Serial.begin(9600);
  lcd.init();                     
  lcd.backlight();
  // Password Protection
  //Serial.println("Enter Password before ")
  //Enter password  
}


void loop() {
  // keypad
  keypads();
  if (error && errorShown == false){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Error");
    errorShown = true;
    return;
  }
  else if (error == false){
    updatedisplayonchange();
    if (calc){
      resetarrays();
      cursorx = 0;
      cursory = 0;
      lcd.setCursor(0,0);
      calc = false;
    }
  }
  //display 

}

void(* resetFunc) (void) = 0;

float calculate(String inputArray[20]) {
  float firstNum;
  float secondNum;
  for(int i = 0; i < 20; i++) { //Bracket
    if (inputArray[i] == "(") {
      int closeIndex = bracketFinder(i);
      String tempArray[20];
      for (int j = i + 1; j < closeIndex; j++) {
        tempArray[j - i + 1] = tempArray[j];
      }
      int value = calculate(tempArray);
      
    }
  }
  for(int i = 0; i < 20; i++) { //Root
    if (inputArray[i] == "R") {
      
    }
  }
  for(int i = 0; i < 20; i++) { //Division
    if (inputArray[i] == "/") {
      
    }
  }
  for(int i = 0; i < 20; i++) {
    if (inputArray[i] == "*") {
      //multiply
      firstNum = floatMaker(inputArray[i - 1]);
      secondNum = floatMaker(inputArray[i + 1]);
      if(error == true) { //If error is detected error should be displayed instead.
        return 0;
      }
      reformArray(inputArray, i, firstNum * secondNum);
      i--;
    }
  }
  for(int i = 0; i < 20; i++) {
    if (inputArray[i] == "+") {
      //add
      firstNum = floatMaker(inputArray[i - 1]);
      secondNum = floatMaker(inputArray[i + 1]);
      if(error == true) { //If error is detected error should be displayed instead.
        return 0;
      }
      reformArray(inputArray, i, firstNum + secondNum);
      i--;
    }
  }
  for(int i = 0; i < 20; i++) {
    if (inputArray[i] == "-") {
      //subtract
      firstNum = floatMaker(inputArray[i - 1]);
      secondNum = floatMaker(inputArray[i + 1]);
      if(error == true) { //If error is detected error should be displayed instead.
        return 0;
      }
      reformArray(inputArray, i, firstNum - secondNum);
      i--;
    }
  }
  if (inputArray[1] == "") { //Only one number is left.
    calc = true;
    return inputArray[0].toFloat();
  }
  else {
    Serial.println("Something is wrong.");
  }
}

float floatMaker(String input) {
  char readin;
  bool dec = false;
  for(int j = 0; j < input.length(); j++) {
    readin = input.charAt(j);
    if (isdigit(readin) == false) {
      if (readin != '.' && readin != '-') { //Not a digit or decimal place
        error = true;
        Serial.println("error set true, invalid char");
        return 0;
      }
      else if (input.charAt(j) == '.' {
        if (dec) {
          error = true;
          Serial.println("error - too many decimals")
          return 0;
        }
        dec = true;
      }
    }
  }
  return input.toFloat(); //Turns input string into float and returns 
}

int bracketFinder(int startBacket) { //Find the indexes of the close of a bracket
  int openBrackets = 1;
  for (int i = startBacket; i < 20; i++) {  
    if (inputArray[i] == "(") {
      openBrackets++;
    }
    else if (openBrackets = ")") {
      openBrackets--;
    }
    if (openBrackets == 0) {
      return i;
    }
  }
}

void reset() {
  resetarrays();
  resetCounters();
}

//array modules
void resetarrays(){
  for (int i = 0; i < 20; i++)
  {
    rawInput[i] = '\0';
    inString[i] = "";
    inStringCopy[i] = "";
    inNum[i] = '\0';
  }
}

void resetCounters() {
  masterptr = 0;
  inNumPos = 0;
  inStringPos = 0;
  cursorx = 0;
  cursory = 0;
}


void reformArray(String *inArray, int index, float newValue) {
  String newArray[20];
  
  for (int i = 0; i < index - 1; i++){ //Enters all of the arrays items upto the one before the first number used
    newArray[i] = inArray[i];
  }
  newArray[index - 1] = newValue;
  for (int i = index; i < 18; i++) { //Moves everything down to fill out the new array.
    newArray[i] = inArray[i + 2];
  }
  newArray[18] = ""; //Prevents random data from getting put into the new array
  newArray[19] = "";

  for (int i = 0; i < 20; i++) { //Copies the temporary array to the orginal.
    inArray[i] = newArray[i];
  }
}

//display functions
void displayitem(char character){
  lcd.setCursor(cursorx,cursory);
  lcd.print(character);
  cursorx++;
}

void printarray(char displayarray[20]){
    cursory = 0;
    cursorx = 0;
  for (int i = 0; i < 20; i++){
    displayitem(displayarray[i]);
  }
}

void updatedisplayonchange(){
  if (displayupdated == false){
    printarray(rawInput);
    displayupdated = true;
  }
}


//Input keypresses
void addtoarrays(char keys){
  if (calc = true) {
    lcd.clear();
    lcd.setCursor(cursorx,cursory);
    calc = false;
  } 

  if (masterptr < 20){
    rawInput[masterptr] = keys;
    if(isDigit(keys) || keys == '.') { //Digit or decimal place
      inNum[inNumPos] = keys;
      inNumPos++;
    }
    else{ //Function key pressed
      String inputtedNumber = "";
      if(inNum[0] != '\0') {  //True if any numbers were entered
        for (int i = 0; i < inNumPos; i++) {
          inputtedNumber += inNum[i];
        }
        for (int i = 0; i < 20; i++) {
          inNum[i] = '\0';
        }
        inString[inStringPos] = inputtedNumber;
        if (keys != '=') {
          inString[inStringPos + 1] = keys;
         inStringPos += 2;
        }
        inNumPos = 0;
      }
      else{ //No numbers were entered eg. double operator
        inString[inStringPos] = keys;
        inStringPos += 1;
      }
      
    }
    masterptr++;
    displayupdated = false;
    // for(int i = 0; i < masterptr + 1; i++) {
    //   Serial.print(rawInput[i]);
    // }
    // Serial.println();
  }
}


void keypads(){
  char key1 = keypad1.getKey();
  char key2 = keypad2.getKey();
  if (on == false && key2 != 'O') {
    return;
  }
  if (key1 != NO_KEY) {
    //Serial.println(key1);
    if (isDigit(key1)){
      //add char to input and display arrays
      addtoarrays(key1);
    } else {
      choosefunckey(key1);
    }
  }
  if (key2 != NO_KEY) {
    //Serial.println(key2);
    if (isDigit(key2)){
      //add char to input and display arrays
      addtoarrays(key2);
    } else {
      choosefunckey(key2);
    }
  }
}


void choosefunckey (char key){
  //has to work this way given buttons have more than one use in some cases
  switch (key)
  {
    case '=':
      //Execute calculator function
      addtoarrays(key);
      for(int i = 0; i < 20; i++) {
        inStringCopy[i] = inString[i];
      }
      float result;
      result = calculate(inStringCopy);
      lcd.setCursor(0,3);
    lcd.println(result);
      reset();
      break;
    case '+':
      //add plus to char array
      addtoarrays(key);
      break;
    case '-':
      //add minus to char array
      addtoarrays(key);
      break;
    case '*':
      // add mulitply to char array
      addtoarrays(key);
      break;
    case '/':
      //Do nothing, not used
      break;
    case '.':
      //add decimal point to char array (make float)
      addtoarrays(key);
      break;
    case 'O':
      resetFunc();
      on = true;
    break;
    case 'F' :
      lcd.noBacklight();
      lcd.clear();
      on = false;
    break;
  }
}