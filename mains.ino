//Import Libraries
#include <LiquidCrystal.h>
#include <Wire.h>
#include <Keypad.h>
#include <math.h>
#include <EEPROM.h>
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

float lastnumber;
String tempLastnumber = "";

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
bool displayupdated = false;
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
    if (password == ActualPassword){
      passwordCorrect = true;
    } else {
      Serial.println("Incorrect");
    }
  }
  lcd.setCursor(0,0);
  //lcd.println(initialText);

  //Menu
  bool menu = true;
  Serial.println("Welcome!");
  disMenuText();
  while (menu) {
    lcd.setCursor(0,0);
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

void timerSetup() {

  //Creates 200ms timer (5Hz)
  cli();

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  OCR1A = 3124; // [16MHz/(1024 * 5Hz)] - 1

  TCCR1B |= (1 << WGM12);

  TCCR1B |= (1 << CS12) | (1 << CS10);

  TIMSK1 |= (1 << OCIE1A);

  sei();
}

ISR(TIMER1_COMPA_vect) {
  keypads(); //Checks keypad every 200ms
  if (interruptCount == 5) { //Runs every second
    adjustbacklight(readLDR());
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

void initEnv(){
  Serial.begin(9600);
  lcd.begin(20,4);                     
  pinMode(BACKLIGHTPIN, OUTPUT);
  adjustbacklight(255);
  pinMode(A0, INPUT); //LDR
  passwordcheck();
  wipememory();
  initialTextCheck();
}

String readSerial(){
  Serial.flush();
  String Output;
  delay(20);
  while (Serial.available() == 0){} 
  while (Serial.available() > 0) {
    Output = Serial.readString();
    Output.replace("\n", "");
    Serial.flush();
    return Output;
  }
}

void adjustbacklight(int lux){
  analogWrite(BACKLIGHTPIN, lux);
}

void passwordcheck(){
  int passwordresetval = EEPROM.read(PasswordAddress + PasswordSpace);//Read last val of password
  Serial.println(passwordresetval);
  if (passwordresetval == 255){
    writeEEPROM(PasswordAddress, PasswordAddress + PasswordSpace - 1, DefaultPassword);
  } 
  ActualPassword = readEEPROM(PasswordAddress,PasswordAddress + PasswordSpace - 1);
  ActualPassword.replace(" ", "");
}
void checkformemory(){
  int memoryresetval = EEPROM.read(MemoryAddress + MemorySpace);//Read last val of memory
  if (memoryresetval == 255){
    wipememory();
  } else {
    String tempMemory = readEEPROM(MemoryAddress, MemoryAddress + MemorySpace - 1);
    Memory = floatMaker(tempMemory);
    EEPROM.write(MemoryAddress+MemorySpace, 0);
  }
}
void wipememory(){
  Memory = 0;
  String strmem = String(Memory);
  writeEEPROM(MemoryAddress, MemoryAddress + MemorySpace - 1, strmem);
}
void MemSaveEEPROM(){
  String strmem = String(Memory);
  writeEEPROM(MemoryAddress, MemoryAddress + MemorySpace - 1, strmem);
  EEPROM.write(MemoryAddress+MemorySpace, 0);
}
void initialTextCheck() {
  int initialTextResetVal = EEPROM.read(initTextAddress + initTextSpace);
  Serial.println(initialTextResetVal);
  if (initialTextResetVal == 255) {
    writeEEPROM(initTextAddress, initTextAddress + initTextSpace - 1, defaultText);
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
  
  while (set){
    String yn = readSerial();
    if (yn == "yes"){
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

    } if (yn == "no"){
      disMenuText();
      lcd.clear();
      return;
    }
    
  }
}  

void disLDR() {
  int light_intensity = readLDR();
  if(EEPROM.read(ldrAddress) == 255) {
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
  adjustbacklight(light_intensity);
  disMenuText();
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
    break;

    case '1': //Temp & humidity mode
      displayTemp();
    break;
      
    case '2':
      displayTime();
    break;

    default:
      Serial.println("Mode char out of range error");
    break;
  }
}

int readLDR() {
  int input = analogRead(ldr);
  int mappedInput = map(input, 8, 1015, 255, 0);
  return mappedInput;
}
  

void(* resetFunc) (void) = 0;

float calculate(String inputArray[20]) {
  float firstNum;
  float secondNum;
  for(int i = 0; i < 20; i++) { //Bracket
    if (inputArray[i] == "(") {
      int closeIndex = bracketFinder(i, inputArray);
      String tempArray[20];
      for (int j = i + 1; j < closeIndex; j++) {
        tempArray[j - i + 1] = tempArray[j];
      }
      float value = calculate(tempArray);
      closeIndex = bracketFinder(i, inputArray);
      reformArray(inputArray, i, closeIndex, value);
      i--;
    }
  }
  for(int i = 0; i < 20; i++) { //Root
    if (inputArray[i] == "R") {
      firstNum = floatMaker(inputArray[i + 1]);
      if (error = true) {
        return 0;
      }
      reformArray(inputArray, i, i + 1, sqrt(firstNum));
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
      i--;
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

int bracketFinder(int startBacket, String inputArray[20]) { //Find the indexes of the close of a bracket
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
  for (int i = startIndex; i < (20 - (closeIndex - startIndex)); i++) { //Moves everything down to fill out the new array.
    newArray[i] = inArray[i + (closeIndex - startIndex)];
  }
  for (int i = 20 - (closeIndex - startIndex); i < 20; i++) {
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
  sprintf(time, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  lcd.setCursor(1, 6);
  lcd.println(time);
}
void lastdig(char key){
  //if not digit or decimal
  //set last number
  if(isDigit(key) || key == '.') { //Digit or decimal place
      tempLastnumber + key;
  } else {
    lastnumber = floatMaker(tempLastnumber);
    tempLastnumber = "";
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
    lastdig(keys);
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
  String Output;
  for (int i = startadd; i <= stopadd; i++){
    char temp = EEPROM.read(i);
    Output = Output + temp;
  }
  Output.trim();
  return Output;
}

void writeEEPROM(int startadd,int stopadd, String msg){
  int j = 0;
  char space = ' ';
  for (int i = startadd; i <= stopadd; i++){
    if (j < msg.length()){
      char temp = msg.charAt(j);
      //Serial.print(i);
      //Serial.println(temp);
      EEPROM.write(i,temp);
      
    } else {
      EEPROM.write(i,space);
    }
    j++;
  }
}
void dellastinput(){
  //take master pointer and retard by 1, delete from all arrays
  subFromArrays();
}

void memrecall(){
  //recall memory from eeprom and input into function
  String memstring = String(Memory);
  for (int i=0; i<memstring.length(); i++){
    char key = memstring.charAt(i);
    addtoarrays(key);
  }
}

void memadd(){
// add last number into memory or whats currently in memory
  Memory = Memory + lastnumber;
  MemSaveEEPROM();
}
void memsubtract(){
 Memory = Memory - lastnumber;
  MemSaveEEPROM();
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
      noInterrupts();
      result = calculate(inStringCopy);
      interrupts();
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
      adjustbacklight(0);
      lcd.clear();
      on = false;
    break;
     case '(':
      addtoarrays(key);
    break;
     case ')':
      addtoarrays(key);
    break;
    case 'R':
      addtoarrays(key);
    break;
    case '%':
      addtoarrays(key);
    break;
    case 'C' :
      //Clear Last digit
      dellastinput();
    break;
    case 'E' :
      //Clear Everything
      reset();
    break;
    case 'Z' :
      //Memory Clear
      wipememory();
    break;
    case 'X' :
      //Memory Recall
      memrecall();
    break;
    case 'V' :
      //Memory Add
      memadd();
    break;
    case 'B' :
      //Memory Subtract
      memsubtract();
    break;
    case '_':
      switch (mode){
        case '0':
          mode = '1';
        break;
        case '1':
          mode = '2';
        break;
        case '2':
          mode = '0';
        break;
      }
    break;
    
  }
}