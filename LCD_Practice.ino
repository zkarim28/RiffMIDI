#include <LiquidCrystal.h>

// Define the pin connections for the LCD
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

// Define the pin connections for left and right buttons
int leftButton = 6;
int rightButton = 7;

// Initialize the library with the pin variables
LiquidCrystal lcd(lcdRS, lcdE, lcdD0, lcdD1, lcdD2, lcdD3, lcdD4, lcdD5, lcdD6, lcdD7);

// Enum to represent custom characters
enum CustomCharacters {
  MUSIC_NOTE = 0,
  EIGHTH_NOTES = 1,
  GREEN = 2,
  RED = 3,
  YELLOW = 4,
  BLUE = 5,
  ORANGE = 6
};

// Define a custom character for a music note
byte musicNote[] = {
  B00100,
  B00110,
  B00101,
  B00100,
  B01100,
  B11100,
  B11100,
  B00000
};

// Define a custom character for two eighth notes
byte eighthNotes[] = {
  B01111,
  B01001,
  B01001,
  B01001,
  B01001,
  B11011,
  B11011,
  B00000
};

byte green[] = {
  B11100,
  B10100,
  B11101,
  B00100,
  B10101,
  B11100,
  B00000,
  B11111
};

byte red[] = {
  B11100,
  B10100,
  B10101,
  B11000,
  B10101,
  B10100,
  B00000,
  B11111
};

byte yellow[] = {
  B10100,
  B10100,
  B01001,
  B01000,
  B01001,
  B01000,
  B00000,
  B11111
};

byte blue[] = {
  B10000,
  B10000,
  B11101,
  B10100,
  B10101,
  B11100,
  B00000,
  B11111
};

byte orange[] = {
  B00000,
  B00000,
  B11101,
  B10100,
  B10101,
  B11100,
  B00000,
  B11111
};

int currentScreen = 0; // Variable to track the current screen
int lastLeftButtonState = HIGH;
int lastRightButtonState = HIGH;

void setup() {
  // Set up the LCD's number of columns and rows
  lcd.begin(16, 2);

  // Set up the buttons as input with internal pull-up resistors enabled
  pinMode(leftButton, INPUT_PULLUP);
  pinMode(rightButton, INPUT_PULLUP);

  // Create custom characters
  lcd.createChar(MUSIC_NOTE, musicNote);
  lcd.createChar(EIGHTH_NOTES, eighthNotes);
  lcd.createChar(GREEN, green);
  lcd.createChar(RED, red);
  lcd.createChar(YELLOW, yellow);
  lcd.createChar(BLUE, blue);
  lcd.createChar(ORANGE, orange);

  // Display the first screen initially
  displayScreen(currentScreen);
}

void loop() {
  int leftButtonState = digitalRead(leftButton);
  int rightButtonState = digitalRead(rightButton);

  // Check if the right button was pressed
  if (rightButtonState == LOW && lastRightButtonState == HIGH) {
    // Switch to the next screen
    currentScreen = (currentScreen + 1) % 3;
    displayScreen(currentScreen);
  }

  // Check if the left button was pressed
  if (leftButtonState == LOW && lastLeftButtonState == HIGH) {
    // Switch to the previous screen
    currentScreen = (currentScreen - 1 + 3) % 3;
    displayScreen(currentScreen);
  }

  // Update the last button states
  lastLeftButtonState = leftButtonState;
  lastRightButtonState = rightButtonState;
}

void displayScreen(int screen) {
  lcd.clear();
  switch (screen) {
    case 0:
      lcd.setCursor(0, 0);
      lcd.print("Chord Buttons ");
      lcd.setCursor(15, 0); // Position at the end of the first line
      lcd.write(byte(EIGHTH_NOTES)); // Display the eighth notes

      lcd.setCursor(0, 1);
      lcd.write(byte(GREEN));
      lcd.print("Bb");
      lcd.setCursor(3, 1);
      lcd.write(byte(RED));
      lcd.print("Cb");
      lcd.setCursor(6, 1);
      lcd.write(byte(YELLOW));
      lcd.print("Db");
      lcd.setCursor(9, 1);
      lcd.write(byte(BLUE));
      lcd.print("Eb");
      lcd.setCursor(12, 1);
      lcd.write(byte(ORANGE));
      lcd.print("Fb");

      break;
    case 1:
      lcd.setCursor(0, 0);
      lcd.print("Note Buttons ");
      lcd.setCursor(15, 0); // Position at the end of the first line
      lcd.write(byte(MUSIC_NOTE)); // Display the music note
      
      lcd.setCursor(0, 1);
      lcd.write(byte(GREEN));
      lcd.print("B");
      lcd.setCursor(3, 1);
      lcd.write(byte(RED));
      lcd.print("C");
      lcd.setCursor(6, 1);
      lcd.write(byte(YELLOW));
      lcd.print("D");
      lcd.setCursor(9, 1);
      lcd.write(byte(BLUE));
      lcd.print("E");
      lcd.setCursor(12, 1);
      lcd.write(byte(ORANGE));
      lcd.print("F");

      break;
    case 2:
      lcd.setCursor(0, 0);
      lcd.print("Misc");
      lcd.setCursor(0, 1);
      lcd.print("Speed: ");
      break;
    default:
      lcd.setCursor(0, 0);
      lcd.print("Error: No Screen");
      break;
  }
}
