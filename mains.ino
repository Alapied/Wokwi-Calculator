//Import Libraries
#include <LiquidCrystal_I2C.h>
#include <Wire.h>


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

//Keypad 2
#define P2R1 32;
#define P2R2 33;
#define P2R3 34;
#define P2R4 35;

#define P2C1 36;
#define P2C2 37;
#define P2C3 38;
#define P2C4 39;

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


void setup() {
  // Initalise Env
  Serial.Begin(9600);
  

  // Password Protection
  
  //Enter password  
}

void loop() {
  // put your main code here, to run repeatedly:

}

float calculate(string[20] inputArray) {
  float firstNum;
  float secNum;
  for(int i = 0; i < (inputArray.length()); i++) {
    if (inputArray[i] == "x") {
      //multiply
    }
    else if (inputArray[i] == "+") {
      //add
    }
    else if (inputArray[i] == "-") {
      //subtract
    }
    else if (firstNum == "") {
      //Nothing in first number
    }
    else{
      //Two numbers entered together
    }
  }
}


//Basic Operands
float add(float num1, float num2) {
  return num1 + num2;
}

float subtract(float num1, float num2) {
  return num1 - num2;
}

float multiply(float num1, float num2) {
  return num1 * num2;
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
    displayitem(rawInput);
  }
}

void updatedisplayonchange(){
  if (displayupdated == false){
    //Update all elements (redraw char array)
    displayupdated == true
  }
}