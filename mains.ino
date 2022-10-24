#include <LiquidCrystal.h>
#include <Wire.h>
#include <Keypad.h>
#include <math.h>
#include <EEPROM.h>
#include <RTClib.h>
#include <dht.h>

//LDR
#define ldr A0

//DHT
#define dhtPin 7

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
  { '^', 'E', '<', '(' },
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


//constants

byte sqrRootSymb[8] = {
  B00000,
  B00011,
  B00010,
  B00010,
  B00010,
  B10010,
  B01010,
  B00100
};
byte smallSubtractSymb[8] = {
  B00000,
  B00000,
  B00000,
  B01111,
  B01111,
  B00000,
  B00000,
  B00000
};


String DefaultPassword = "EEE20003";
String ActualPassword = "";
String defaultText = "Group 8 Calculator";
String initialText = "";
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
int interruptCount = 1;
char mode = '0'; //0 is calc, 1 is temp/humidity, 2 is time.

float result = 0.00;
float lastNumber;
String tempLastnumber = "";
char lastOperand = '+'; 
int lastOperandLocation = 0;

int humidity = 0;
float temp = 0.0;
DateTime past;

//EEPROM
float Memory;
int MemoryAddress = 22;
int MemorySpace = 21; // last digit is the reset bool
int PasswordAddress = 0;
int PasswordSpace = 21;
int initTextAddress = 44;
int initTextSpace = 21;
int ldrAddress = 66;
int ldrSpace = 21;

//Global Bools
bool displayUpdated = false;
bool displayDHTUpdated = true;
bool displayClockUpdated = true;
bool calc = false;
bool on = true;
bool error = false;
bool errorShown = false;

//Device Int
#define RS 13
#define EE 12
#define D4 11
#define D5 10
#define D6 9
#define D7 8
#define BACKLIGHTPIN 6
LiquidCrystal lcd(RS, EE, D4, D5, D6, D7);
Keypad keypad1 = Keypad(makeKeymap(keys1), rowPins1, colPins1, ROWS, COLS);
Keypad keypad2 = Keypad(makeKeymap(keys2), rowPins2, colPins2, ROWS, COLS);
RTC_DS1307 rtc;
dht DHT;

void setup() {
  // Initalise Env
  initEnv();
  // Password Protection
  bool passwordCorrect = false;
  while (passwordCorrect == false) {
    Serial.println("Enter Password: ");
    //Enter password
    String password = readSerial();
    password.replace(" ", "");
    Serial.println(password);
    if (password == ActualPassword) {
      passwordCorrect = true;
    } else {
      Serial.println("Incorrect");
    }
  }
  lcd.setCursor(0, 0);
  //lcd.println(initialText);

  //Menu
  bool menu = true;
  Serial.println("Welcome!");
  disMenuText();
  while (menu) {
    lcd.setCursor(0, 0);
    lcd.print(initialText);
    String input = readSerial();
    switch (input.charAt(0)) {
      case '1':
        Serial.println("Change Init text mode");
        changeInitText();
        break;
      case '2':
        Serial.println("LDR Display mode");
        disLDR();
        break;
      case '3':
        Serial.println("Calculator mode");
        menu = false;
        timerSetup();
        lcd.clear();
        break;
    }
  }


}

//Interrupt ISR

void timerSetup() {

  //Creates 100ms timer (5Hz)
  cli();

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  OCR1A = 1562.5; // [16MHz/(1024 * 10Hz)] - 1

  TCCR1B |= (1 << WGM12);

  TCCR1B |= (1 << CS12) | (1 << CS10);

  TIMSK1 |= (1 << OCIE1A);

  sei();
}

ISR(TIMER1_COMPA_vect) {
  keypads(); //Checks keypad every 100ms
  if (interruptCount == 10) { //Runs every second
    adjustBacklight(readLDR());
    interruptCount = 0;
  }
  interruptCount++;
}

void disMenuText() {
  Serial.println("Please select an option.");
  Serial.println("1: Change default LCD text.");
  Serial.println("2: Display LDR sensor data.");
  Serial.println("3: Enter the calculator");
}

void initEnv() {
  Serial.begin(9600);
  lcd.begin(20, 4);
  pinMode(BACKLIGHTPIN, OUTPUT);
  adjustBacklight(255);
  pinMode(A0, INPUT); //LDR
  passwordcheck();
  wipememory();
  initialTextCheck();
  createCustomChars();
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
}
void createCustomChars() {
  lcd.createChar(3, sqrRootSymb);
  lcd.createChar(4, smallSubtractSymb);
}
String readSerial() {
  Serial.flush();
  String Output;
  delay(20);
  while (Serial.available() == 0) {}
  while (Serial.available() > 0) {
    Output = Serial.readString();
    Output.replace("\n", "");
    Serial.flush();
    return Output;
  }
}
//Backlight and LDR
void adjustBacklight(int lux) {
  analogWrite(BACKLIGHTPIN, lux);
}

void disLDR() {
  int light_intensity = readLDR();
  if (EEPROM.read(ldrAddress) == 255) {
    EEPROM.write(ldrAddress, 1);
    writeEEPROM(ldrAddress + 1, ldrAddress + ldrSpace, String(light_intensity));
    Serial.print("Current LDR value of ");
    Serial.print(light_intensity);
    Serial.println(" saved to the EEPROM.");
  }

  String ldrValue = readEEPROM(ldrAddress + 1, ldrAddress + ldrSpace);
  Serial.print(ldrValue);
  Serial.println(" was read from the EEPROM!");
  Serial.print("Current value is ");
  Serial.println(readLDR());
  adjustBacklight(light_intensity);
  disMenuText();
}
int readLDR() {
  int input = analogRead(ldr);
  int mappedInput = map(input, 8, 1015, 255, 0);
  return mappedInput;
}
// Temp and Clock
void readsensor(){
  DHT.read22(dhtPin);
  int inHumidity = DHT.humidity;
  float inTemp = DHT.temperature;
  if ((inHumidity == humidity) || (inTemp == temp)){
    // do nothing
  } else {
    humidity = inHumidity;
    temp = inTemp;
    displayDHTUpdated = false;
  }
  
}

void displayTemp() {
  readsensor();
  if (displayDHTUpdated == false){
    lcd.setCursor(0, 2);
    lcd.print(temp);
    lcd.print("C");
    lcd.setCursor(0, 1);
    lcd.print(humidity);
    lcd.print("%");
    displayDHTUpdated = true;
  } 
}

void displayTime() {
  DateTime current = rtc.now();
  clockTimeCheck();
  if (displayClockUpdated == false){
    past = current;
    lcd.setCursor(6, 2);
    char time[8];
    sprintf(time, "%02d:%02d:%02d", current.hour(), current.minute(), current.second());
    lcd.println(time);
    displayClockUpdated = true;
  }
}
void clockTimeCheck(){
  DateTime current = rtc.now();
  if ((current.hour()==past.hour())&&(current.minute()==past.minute()) && (current.second()==past.second())){
    //Serial.println("time same");
  }else {
    displayClockUpdated = false;
    past = current;
  }
  
}
//Password
void passwordcheck() {
  //Read last val of password
  int passwordresetval = EEPROM.read(PasswordAddress + PasswordSpace);
  //Checks if a value has been entered already for the password
  if (passwordresetval == 255) {
    writeEEPROM(PasswordAddress, PasswordAddress + PasswordSpace - 1, DefaultPassword);
    EEPROM.write(PasswordAddress + PasswordSpace, 0);
  }
  ActualPassword = readEEPROM(PasswordAddress, PasswordAddress + PasswordSpace - 1);
  ActualPassword.replace(" ", "");
}
//memory
void checkformemory() {
  int memoryresetval = EEPROM.read(MemoryAddress + MemorySpace);//Read last val of memory
  if (memoryresetval == 255) {
    wipememory();
  } else {
    String tempMemory = readEEPROM(MemoryAddress, MemoryAddress + MemorySpace - 1);
    Memory = floatMaker(tempMemory);
    EEPROM.write(MemoryAddress + MemorySpace, 0);
  }
}
void wipememory() {
  Memory = 0;
  String strmem = String(Memory);
  writeEEPROM(MemoryAddress, MemoryAddress + MemorySpace - 1, strmem);
}
void MemSaveEEPROM() {
  String strmem = String(Memory);
  writeEEPROM(MemoryAddress, MemoryAddress + MemorySpace - 1, strmem);
  EEPROM.write(MemoryAddress + MemorySpace, 0);
}
void memrecall() {
  //recall memory from eeprom and input into function
  String memstring = String(Memory);
  for (int i = 0; i < memstring.length(); i++) {
    char key = memstring.charAt(i);
    addKeytoArray(key);
  }
}

void memadd() {
  // add last number into memory or whats currently in memory
  Memory = Memory + result;
  Serial.print(Memory);
  MemSaveEEPROM();
}
void memsubtract() {
  Memory = Memory - result;
  Serial.print(Memory);
  MemSaveEEPROM();
}
//EEPROM
String readEEPROM(int startAdd, int stopAdd) {
  String outputText;
  for (int i = startAdd; i <= stopAdd; i++) {
    char temp = EEPROM.read(i);
    outputText += temp;
  }
  outputText.trim();
  return outputText;
}

void writeEEPROM(int startadd, int stopadd, String msg) {
  int j = 0;
  char space = ' ';
  for (int i = startadd; i <= stopadd; i++) {
    if (j < msg.length()) {
      char temp = msg.charAt(j);
      //Serial.print(i);
      //Serial.println(temp);
      EEPROM.write(i, temp);

    } else {
      EEPROM.write(i, space);
    }
    j++;
  }
}
//inital text

void initialTextCheck() {
  int initialTextResetVal = EEPROM.read(initTextAddress + initTextSpace);
  if (initialTextResetVal == 255) {
    Serial.println("No value stored, loading in default for inital text.");
    writeEEPROM(initTextAddress, initTextAddress + initTextSpace - 1, defaultText);
    EEPROM.write(initTextAddress + initTextSpace, 0);
  }
  initialText = readEEPROM(initTextAddress, initTextAddress + initTextSpace - 1);
  Serial.println(initialText);
}

void changeInitText() {
  bool set = true;
  Serial.println("Please enter the new text, it must be 20 characters or less.");
  String input = readSerial();
  Serial.println(input);
  Serial.println("Do you want to Save this Text? [yes/no]");

  while (set) {
    String yn = readSerial();
    if (yn == "yes") {
      if (input.length() <= 20) {
        initialText = input;
        writeEEPROM(initTextAddress, initTextAddress + initTextSpace - 1, initialText);
        Serial.println("Saved to EEPROM");
        disMenuText();
        lcd.clear();
        return;
      }
      else {
        Serial.println("Text was too long, it was not saved!");
      }

    } if (yn == "no") {
      disMenuText();
      lcd.clear();
      return;
    }

  }
}

void loop() {
  switch (mode) {
    case '0': //Calc mode
      if (error && errorShown == false) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Error");
        errorShown = true;
        return;
      }
      else if (error == false) {
        updatedisplayonchange();
        //Serial.println("Updated display");
      }
      delay(50);
      break;

    case '1': //Temp & humidity mode
      //lcd.clear();
      displayTemp();
      delay(50);
      break;

    case '2':
      //lcd.clear();
      displayTime();
      delay(50);
      break;

    default:
      Serial.println("Mode char out of range error");
      break;
  }
}

//calculate
String calculate(String *inputArray) {
  float firstNum;
  float secondNum;
  for (int i = 0; i < inStringPos; i++) { //Brackets
    if (inputArray[i] == "(") {
      int closeIndex = bracketFinder(i, inputArray);
      String tempArray[20];
      for (int j = i; j < closeIndex - 1; j++) {
        tempArray[j - i] = inputArray[j + 1];
      }
      if (error == true) {
        return "0";
      }
      String value = calculate(tempArray);
      closeIndex = bracketFinder(i, inputArray);
      reformArray(inputArray, i, closeIndex, value);
      i--;
    }
  }
  for (int i = 0; i < inStringPos; i++) { //Root & indicies
    if (inputArray[i] == "R" || inputArray[i] == "^") {
      if (inputArray[i] == "R") { 
        //Root

        //Checks if the number after the root symbol is empty
        if (emptyChecker(inputArray[i + 1])) {
          error = true;
          return "0";
        }
        //Assigns firstNum to the number after the root.
        firstNum = floatMaker(inputArray[i + 1]);
        if (error == true) {
          return "0";
        }

        //Checks if sqaure root would be of a negative number.
        if (firstNum < 0) {
          error = true;
          return "0";
        }
        //Reforms the arrays with square root of firstNum.
        reformArray(inputArray, i, i + 1, String(pow(firstNum, 0.5)));
      }
      else {
        //Power function.
        if (emptyChecker(inputArray[i - 1]) || emptyChecker(inputArray[i + 1])) {
          error = true;
          return "0";
        }
        firstNum = floatMaker(inputArray[i - 1]);
        secondNum = floatMaker(inputArray[i + 1]);

        //Out of range checking to prevent nan errors

        //Checks if there is a decimal component, to prevent root of a negative number
        if (secondNum - floorf(secondNum) != 0) { 
          //Negative number in the root.
          if (firstNum < 0) { 
            error = true;
            return "0";
          }
        }
        //Checks for 0^0;
        if (secondNum == 0 && firstNum == 0 ) {
          error = true;
          return "0";
        }
        reformArray(inputArray, i - 1, i + 1, String(pow(firstNum, secondNum)));
        i--;
      }

    }
  }
  for (int i = 0; i < inStringPos; i++) { //Division
    if (inputArray[i] == "/") {
      if (emptyChecker(inputArray[i - 1]) || emptyChecker(inputArray[i + 1])) {
        error = true;
        return "0";
      }
      firstNum = floatMaker(inputArray[i - 1]);
      secondNum = floatMaker(inputArray[i + 1]);
      if (secondNum == 0) {
        error = true;
        Serial.println("Div by 0 error.");
      }
      if (error == true) {
        return "0";
      }
      reformArray(inputArray, i - 1, i + 1, String(firstNum / secondNum));
      i--;
    }
  }
  for (int i = 0; i < inStringPos; i++) {
    if (inputArray[i] == "*") {
      //multiply
      if (emptyChecker(inputArray[i - 1]) || emptyChecker(inputArray[i + 1])) {
        error = true;
        return "0";
      }
      firstNum = floatMaker(inputArray[i - 1]);
      secondNum = floatMaker(inputArray[i + 1]);
      if (error == true) { //If error is detected error should be displayed instead.
        return "0";
      }
      reformArray(inputArray, i - 1, i + 1, String(firstNum * secondNum));
      i--;
    }
  }
  for (int i = 0; i < inStringPos; i++) {
    //Checking for negative or positive signs next to each other
    if (inputArray[i] == "-") {
      //-- -> +
      if (inputArray[i - 1] == "-") {
        reformArray(inputArray, i - 1, i, "+");
        i--;
      }
      // +- -> -
      else if (inputArray[i - 1] == "+") {
        reformArray(inputArray, i - 1, i, "-");
        i--;
      }

      //-- -> +
      if (inputArray[i + 1] == "-") {
        reformArray(inputArray, i, i + 1, "+");
      }
      // -+ -> -
      else if (inputArray[i + 1] == "+") {
        reformArray(inputArray, i, i + 1, "-");
      }
    }
  }
  for (int i = 0; i < inStringPos; i++) {
    if (inputArray[i] == "+") {
      //add
      if (emptyChecker(inputArray[i - 1]) || emptyChecker(inputArray[i + 1])) {
        error = true;
        return "0";
      }
      firstNum = floatMaker(inputArray[i - 1]);
      secondNum = floatMaker(inputArray[i + 1]);
      if (error == true) { //If error is detected error should be displayed instead.
        return "0";
      }
      reformArray(inputArray, i - 1, i + 1, String(firstNum + secondNum));
      i--;
    }
  }
  for (int i = 0; i < inStringPos; i++) {
    if (inputArray[i] == "-") {
      //subtract
      if (emptyChecker(inputArray[i - 1]) || emptyChecker(inputArray[i + 1])) {
        error = true;
        return "0";
      }
      firstNum = floatMaker(inputArray[i - 1]);
      secondNum = floatMaker(inputArray[i + 1]);
      if (error == true) { //If error is detected error should be displayed instead.
        return "0";
      }
      reformArray(inputArray, i - 1, i + 1, String(firstNum - secondNum));
      i--;
    }
  }
  //Serial.println(inputArray[1]);
  if (inputArray[1] == "") { //Only one number is left.
    calc = true;
    return inputArray[0];
  }
  else {
    Serial.println("Something is wrong.");
  }
}

//Array Modifiers
float floatMaker(String input) {
  char readin;
  bool dec = false;
  for (int j = 0; j < input.length(); j++) {
    readin = input.charAt(j);
    if (isdigit(readin) == false) {
      if (readin != '.' && readin != '-') { //Not a digit or decimal place
        error = true;
        Serial.println("error set true, invalid char");
        return 0;
      }
      else if (input.charAt(j) == '.') {
        if (dec) {
          error = true;
          Serial.println("error - too many decimals");
          return 0;
        }
        dec = true;
      }
    }
  }
  return input.toFloat(); //Turns input string into float and returns
}

void reformArray(String *inArray, int startIndex, int closeIndex, String newValue) {
  String newArray[20];
  if (startIndex != 0) {
    for (int i = 0; i < startIndex; i++) { //Enters all of the arrays items upto the one before the first number used
      newArray[i] = inArray[i];
    }
  }
  newArray[startIndex] = newValue;
  for (int i = startIndex + 1; i < (20 - (closeIndex - startIndex)); i++) { //Moves everything down to fill out the new array.
    newArray[i] = inArray[i + (closeIndex - startIndex)];
  }
  for (int i = 20 - (closeIndex - startIndex); i < 20; i++) {
    newArray[i] = ""; //Prevents random data from getting put into the new array
  }
  for (int i = 0; i < 20; i++) { //Copies the temporary array to the orginal.
    inArray[i] = newArray[i];
  }
}

int bracketFinder(int startBacket, String *inputArray) { //Find the indexes of the close of a bracket
  int openBrackets = 0;
  for (int i = startBacket; i < 20; i++) {
    if (inputArray[i] == "(") {
      openBrackets++;
    }
    else if (inputArray[i] == ")") {
      openBrackets--;
      if (openBrackets == 0) {
        return i;
      }
    }

    if (i == 19) {
      if (openBrackets > 0) {
        error = true;
        Serial.println("Not enough close brackets");
      }

      if (openBrackets < 0) {
        error = true;
        Serial.println("Too many open brackets");
      }
    }
  }
}

bool emptyChecker(String num) {
  if (num == "") {
    error = true;
    return true;
  }
  else {
    return false;
  }
}

void addKeytoArray(char keys) {
  if (calc) {
    lcd.clear();
    lcd.setCursor(cursorx, cursory);
    calc = false;
  }

  if (masterptr >= 20) {
    return;
  }
  
  rawInput[masterptr] = keys;
  lastDigit(keys, masterptr);
  //Serial.print("Added key: ");
  //Serial.println(rawInput[masterptr]);
  
  if (isDigit(keys) || keys == '.') { //Digit or decimal place
    inNum[inNumPos] = keys;
    inNumPos++;
  }
  else { //Function key pressed
    String inputtedNumber = "";
    if (inNumPos != 0) { //True if any numbers were entered
      for (int i = 0; i < inNumPos; i++) {
        inputtedNumber += inNum[i];
      }
      for (int i = 0; i < 20; i++) {
        inNum[i] = '\0';
      }
      inString[inStringPos] = inputtedNumber;
      inStringPos++;
      if (keys != '=') {
        inString[inStringPos] = keys;
        inStringPos++;
      }
      inNumPos = 0;
    }
    else { //No numbers were entered eg. double operator
      if (keys != '=') {
        inString[inStringPos] = keys;
        inStringPos += 1;
      }
    }

  }
  masterptr++;
  displayUpdated = false;
  // for(int i = 0; i < masterptr + 1; i++) {
  //   Serial.print(rawInput[i]);
  // }
  // Serial.println();
}

void removeLastCharacter() {
  if (calc) {
    lcd.clear();
    lcd.setCursor(cursorx, cursory);
    calc = false;
  }

  if (masterptr > 0) {
    masterptr--;
    rawInput[masterptr] = '\0';
    displayUpdated = false;

    if (inNumPos != 0) {
      inNumPos--;
      inNum[inNumPos] = '/0';
      displayUpdated = false;
    }
    else {
      inStringPos--;
      String currentNum = inString[inStringPos];
      if (isDigit(currentNum.charAt(0))) {
        int len = currentNum.length();
        currentNum.remove(len - 1);
        inString[inStringPos] = "";
        for (int i = 0; i < len; i++) {
          inNum[i] = currentNum.charAt(i);
        }
        inNumPos = len - 1;
      }
      else {
        inString[inStringPos] = "";
      }
      displayUpdated = false;
    }
  }
}

void flipNumberOperand(){
  //Nothing is is inNum
  if (inNumPos == 0) {
    inNum[inNumPos] = '-';
    inNumPos++;
    int j = 0;
    for (int i = masterptr - inNumPos + 1; i < masterptr + 1; i++) {
      rawInput[i] = inNum[j];
      j++;
    }
    masterptr += 1;
  }
  //inNum is full
  else if (inNumPos == 20) {
    error = true;
    return;
  }
  //Checks if inNum already has a negative at the start

  //Number is not already negative
  else if (inNum[0] != '-') {
    //Moves all items up one position
    for (int i = inNumPos; i > 0; i--){
      inNum[i] = inNum[i - 1];
    }
    inNum[0] = '-';
    inNumPos++;

    int j = 0;
    for (int i = masterptr - inNumPos + 1; i < masterptr + 1; i++) {
      rawInput[i] = inNum[j];
      j++;
    }
    masterptr += 1;
  }
  //Number is already negative
  else if (inNum[0] == '-') {
    //Shuffles all items down one position
    for (int i = 0; i < inNumPos; i++){
      inNum[i] = inNum[i + 1];
    }
    inNumPos--;

    int j = 0;
    for (int i = masterptr - inNumPos - 1; i < masterptr + 1; i++) {
      rawInput[i] = inNum[j];
      j++;
    }
    masterptr -= 1;
  }

  displayUpdated = false;
}

//reset functions
void resetarrays() {
  for (int i = 0; i < 20; i++)
  {
    rawInput[i] = '\0';
    inString[i] = "";
    inStringCopy[i] = "";
    inNum[i] = '\0';
  }
}
void reset(bool full) {
  resetarrays();
  resetCounters();
  lcd.clear();
  if (full) {
    error = false;
  }
}
void resetCounters() {
  masterptr = 0;
  inNumPos = 0;
  inStringPos = 0;
  cursorx = 0;
  cursory = 0;
}

void(* resetFunc) (void) = 0;

//display functions
void displayitem(char character) {
  char modifiedCharacter;
  switch (character){
    case 'R':
      modifiedCharacter = '\x03';
    break;
    case '*':
      modifiedCharacter ='X';
    break;
    case '<':
      modifiedCharacter = '\x04';
    break;
    default:
      modifiedCharacter = character;
    break;
  }
  lcd.setCursor(cursorx, cursory);
  lcd.print(modifiedCharacter);
  cursorx++;
}

void printarray(char displayarray[20]) {
  cursory = 0;
  cursorx = 0;
  for (int i = 0; i < 20; i++) {
    displayitem(displayarray[i]);
    //Serial.print("Drawing text on screen: ");
    //Serial.println(displayarray[i]);
  }
}

void updatedisplayonchange() {
  if (displayUpdated == false) {
    printarray(rawInput);
    displayUpdated = true;
  }
}

void lastDigit(char key, int loc) {
  //if not digit or decimal
  //set last number
  if (isDigit(key) || key == '.') { //Digit or decimal place
    tempLastnumber + key;
  } else {
    lastNumber = floatMaker(tempLastnumber);
    tempLastnumber = "";
  }
  if((key == '+') || (key == '-')){
    lastOperand = key;
    lastOperandLocation = loc;
  }
}
//Input keypresses

void keypads() {
  switch (mode){
    case 0:
      char key1 = keypad1.getKey();
  char key2 = keypad2.getKey();
  if (on == false && key2 != 'O') {
    return;
  }
  if (key1 != NO_KEY) {
    //Serial.println(key1);
    if (error) {
      reset(true);
    }
    //Serial.println(key1);
    if (isDigit(key1)) {
      //add char to input and display arrays
      addKeytoArray(key1);
    } else {
      chooseFunctionKey(key1);
    }
  }
  if (key2 != NO_KEY) {
    //Serial.println(key2);
    if (error) {
      reset(true);
    }
    //Serial.println(key2);
    if (isDigit(key2)) {
      //add char to input and display arrays
      addKeytoArray(key2);
    } else {
      chooseFunctionKey(key2);
    }
  }
    break;
    default:
    char key1 = keypad1.getKey();
    char key2 = keypad2.getKey();
      if (on == false && key2 != 'O') {
    return;
    }
    if (key1 != NO_KEY) {
    //Serial.println(key1);
      if (error) {
        reset(true);
    }
    chooseFunctionKey(key1)
    }
    if (key2 != NO_KEY) {
    //Serial.println(key1);
      if (error) {
        reset(true);
    }
    chooseFunctionKey(key2)
    }
    break;
  }
  
}


void chooseFunctionKey (char key) {
  //has to work this way given buttons have more than one use in some cases
  switch (mode){
  case 0:
    switch (key)
    {
      case '=':
        //Execute calculator function
        addKeytoArray(key);
        for (int i = 0; i < 20; i++) {
         inStringCopy[i] = inString[i];
        }
        noInterrupts();
        if (calc) {
          resetarrays();
        cursorx = 0;
        cursory = 0;
        lcd.setCursor(0, 0);
        calc = false;
      }
      result = calculate(inStringCopy).toFloat();
      interrupts();
      reset(false);
      lcd.setCursor(0, 3);
      lcd.println(result);
    break;
    case '_':
      switch (mode) {
        case '0':
          mode = '1';
          lcd.clear();
        break;
        case '1':
          mode = '2';
          lcd.clear();
        break;
        case '2':
          mode = '0';
          lcd.clear();
        break;
      }
    break;
    case 'O':
      resetFunc();
    break;
    case 'F' :
      adjustBacklight(0);
      lcd.clear();
      on = false;
    break;
    case 'R':
      addKeytoArray(key);
      addKeytoArray('(');
    break;
    case 'C' :
      //Clear Last digit
      if (error || calc) {
        reset(true);
      }
      else {
        removeLastCharacter();
      }
    break;
    case 'E' :
      //Clear Everything
      reset(true);
    break;
    case 'Z' :
      //Memory Clear
      wipememory();
    break;
    case 'X' :
      //Memory Recall
      memrecall();
    break;
    case 'B' :
      //Memory Add
      memadd();
    break;
    case 'V' :
      //Memory Subtract
      memsubtract();
    break;
    case '^':
      addKeytoArray(key);
      addKeytoArray('(');
    break;
    case '!':
      //Empty key
    break;
    case '<':
      flipNumberOperand();
    break;
    default:
      //All other keys
      addKeytoArray(key);
    break;
  }
  break;
  default:
  switch (key)
    {
    case 'O':
      resetFunc();
    break;
    case 'F' :
      adjustBacklight(0);
      lcd.clear();
      on = false;
    break;
    case 'E' :
      //Clear Everything
      reset(true);
    break;
    case 'Z' :
      //Memory Clear
      wipememory();
    break;
  }
  break;
  }
}