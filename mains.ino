//Import Libraries
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Keypad.h>
#include <math.h>
#include <EEEPROM.h>
#include <RTClib.h>
#include <dht.h>


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
  { 'T', 'E', '<', '(' },
  { 'R', 'Z', 'X', ')' },
  { '%', 'V', 'B', '_' },
  { 'O', 'C', 'F', '!' }
};
//Z MC
//X MR
//V M-
//B M+
//_ Fn
uint8_t colPins2[COLS] = { P2C1, P2C2, P2C3, P2C4 }; // Pins connected to C1, C2, C3, C4
uint8_t rowPins2[ROWS] = { P2R1, P2R2, P2R3, P2R4 }; // Pins connected to R1, R2, R3, R4

//RTC init
RTC_DS1307 rtc;

//DHT init
dht DHT;

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
char mode = '0'; //0 is calc, 1 is temp/humidity, 2 is time.

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
  initEnv();
  // Password Protection
  //Serial.println("Enter Password before ")
  //Enter password  
}
void initEnv(){
  Serial.begin(9600);
  lcd.init();                     
  lcd.backlight();
  interruptsetup();
}
void interruptsetup(){
  for (int i = 22; i <=25; i++){
    attachInterrupt(digitalPinToInterrupt(i), keypads(), RISING)
  }
  for (int j = 32; j<=35; j++){
    attachInterrupt(digitalPinToInterrupt(j), keypads(), RISING)
  }
}
void loop() {
  switch (mode) {
    case '0': //Calc mode
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

    case '1': //Temp & humidity mode
      displayTemp();
      
    case '2':
      displayTime();

    default:
      Serial.println("Mode char out of range error");

  }
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
      float value = calculate(tempArray);
      closeIndex = bracketFinder(i);
      reformArray(inputArray, i, closeIndex, value);
    }
  }
  for(int i = 0; i < 20; i++) { //Root
    if (inputArray[i] == "R") {
      firstNum = floatMaker(inputArray[i + 1]);
      if (error = true) {
        return 0;
      }
      reformArray(inputArray, i, i + 1; sqrt(firstNum));
    }
  }
  for(int i = 0; i < 20; i++) { //Division
    if (inputArray[i] == "/") {
      firstNum = floatMaker(inputArray[i - 1]);
      secondNum = floatMaker(inputArray[i - 1]);
      if (error = true) {
        return 0;
      }
      reformArray(inputArray, i, i + 1, firstNum / secondNum);
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
      reformArray(inputArray, i, i + 1, firstNum * secondNum);
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
      reformArray(inputArray, i, i + 1, firstNum + secondNum);
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
      reformArray(inputArray, i, i + 1, firstNum - secondNum);
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


void reformArray(String *inArray, int startIndex, int closeIndex, float newValue) {
  String newArray[20];
  
  for (int i = 0; i < startIndex - 1; i++){ //Enters all of the arrays items upto the one before the first number used
    newArray[i] = inArray[i];
  }
  newArray[startIndex - 1] = newValue;
  for (int i = startIndex; i < (20 - (closeIndex - startIndex); i++) { //Moves everything down to fill out the new array.
    newArray[i] = inArray[i + (closeIndex - startIndex)];
  }
  for (int i = 20 - (closeIndex - startIndex); i < 20; i++ {
    newArray[i] = ""; //Prevents random data from getting put into the new array
  }
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

void displayTemp() {
  DHT.read22(7);
  int humidity = DHT.humidity;
  float temp = DHT.temperature;
  if (temp < 0) {
    lcd.setCursor(1, 7);
    lcd.print(temp);
    lcd.print("C");
  }
  else {
    lcd.setCursor(1, 8);
    lcd.print(temp);
    lcd.print("C");
  }

  if (temp == 100) {
    lcd.setCursor(2, 7);
    lcd.print(humidity);
    lcd.print("%");
  }
  else {
    lcd.setCursor(2, 8);
    lcd.print(humidity);
    lcd.print("%");
  }
}

void displayTime() {
  DateTime now = rtc.now();
  char time[20];
  sprintf(time, "%02d:%02d:%02d", now.hour(), now.minute(), now.second())
  lcd.setCursor(1, 6);
  lcd.println(time);
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

void subFromArrays(){
  if (calc = true) {
    lcd.clear();
    lcd.setCursor(cursorx,cursory);
    calc = false;
  }
  if (masterptr > 0){
    masterptr--;
    rawInput[masterptr] = '\0';
    displayupdated = false;
  }
  if (inNum[0] != '\0') {
    inNumPos--;
    inNum[inNumPos] = '/0';
    displayupdated = false;

  }
  else {
    inStringPos--;
    inString[inStringPos] = "";
  }
}

String readEEPROM(int startadd,int stopadd){
  string Output = "";
  for (i = startadd; i <= stopadd; i++){
    char temp = EEPROM.read(i);
    Output + temp;
  }
  return Output;
}

void writeEEPROM(int startadd,int stopadd, String msg){
  int j = 0;
  for (i = startadd; i <= stopadd; i++){
    if (j < msg.length()){
      char temp = msg.charAt(j);
      EEPROM.write(i,temp);
    }
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
      addtoarrays(key);
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
    case 'C' :
      //Clear Last digit
      
    break;
    case 'E' :
      //Clear Everything
      reset();
    break;
    case 'Z' :
      //Memory Clear
    break;
    case 'X' :
      //Memory Recall
    break;
    case 'V' :
      //Memory Add
    break;
    case 'B' :
      //Memory Subtract
    break;
    
  }
}