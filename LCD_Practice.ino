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

// Initialize the library with the pin variables
LiquidCrystal lcd(lcdRS, lcdE, lcdD0, lcdD1, lcdD2, lcdD3, lcdD4, lcdD5, lcdD6, lcdD7);

int currentScreen = 0; // Variable to track the current screen

void setup() {
  // Set up the LCD's number of columns and rows
  lcd.begin(16, 2);
  Serial.begin(9600); // Start serial communication

  // Display the first screen initially
  displayScreen(currentScreen);
}

void loop() {
  if (Serial.available() > 0) {
    char input = Serial.read(); // Read the incoming serial data
    
    if (input == 'r') {
      // Switch to the next screen
      currentScreen = (currentScreen + 1) % 2;
      displayScreen(currentScreen);
    }
    
    if (input == 'l') {
      // Switch to the previous screen
      currentScreen = (currentScreen - 1 + 2) % 2;
      displayScreen(currentScreen);
    }
  }
}

void displayScreen(int screen) {
  lcd.clear();
  switch (screen) {
    case 0:
      lcd.setCursor(0, 0);
      lcd.print("......");
      lcd.setCursor(0, 1);
      lcd.print("RockBand Guitar");
      break;
    case 1:
      lcd.setCursor(0, 0);
      lcd.print("Screen 2 Line 1");
      lcd.setCursor(0, 1);
      lcd.print("Screen 2 Line 2");
      break;
    default:
      lcd.setCursor(0, 0);
      lcd.print("Error: No Screen");
      break;
  }
}