#include <LiquidCrystal.h>

// Define the pin connections using int variables
int lcdRS = 22;
int lcdE = 24;
int lcdD7 = 23;
int lcdD6 = 25;
int lcdD5 = 27;
int lcdD4 = 29;
int lcdD3 = 31;
int lcdD2 = 33;
int lcdD1 = 35;
int lcdD0 = 37;

//VDD -> 5V
//VSS -> GND
//R/W -> GND
//A -> 5V
//K -> GND
//V0 to 10K pot

// Initialize the library with the pin variables
LiquidCrystal lcd(lcdRS, lcdE, lcdD0, lcdD1, lcdD2, lcdD3, lcdD4, lcdD5, lcdD6, lcdD7);

void setup() {
  // Set up the LCD's number of columns and rows
  lcd.begin(16, 2);

  // Print the first line
  lcd.setCursor(0, 0);
  lcd.print("......");

  // Print the second line
  lcd.setCursor(0, 1);
  lcd.print("RockBand Guitar");
}

void loop() {
  // No need to repeat anything in loop
}