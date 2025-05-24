#include <Control_Surface.h>
#include <MIDI_Constants/Chords/Chords.hpp>
#include <LiquidCrystal.h>
#define MAX_CHORD_SIZE 6  // Set this to be the largest chord size

USBMIDI_Interface midi;

class MyChordButton : public MIDIOutputElement {
 public:
  MyChordButton(pin_t notePin, pin_t strumUpPin, pin_t strumDownPin, int chord, uint8_t* chordSize, uint8_t* octavePtr, uint8_t* velocityPtr, uint8_t* strumSpeed)
    : noteButton(notePin), strumUpSwitch(strumUpPin), strumDownSwitch(strumDownPin), chord(chord), chordSize(chordSize), octavePtr(octavePtr), velocityPtr(velocityPtr), strumSpeed(strumSpeed) {}

  void begin() final override { 
    noteButton.begin(); 
    strumUpSwitch.begin();
    strumDownSwitch.begin();
  }

  void update() final override {
    AH::Button::State noteState = noteButton.update();
    AH::Button::State strumUpState = strumUpSwitch.update();
    AH::Button::State strumDownState = strumDownSwitch.update();

    if (noteState == AH::Button::Pressed && strumDownState == AH::Button::Falling) {
      // Control_Surface.sendNoteOn({note[*octavePtr], Channel_1}, *velocityPtr);  // Use the current velocity value
      chordDown(chord, chordSize, octavePtr, velocityPtr);
    } else if (noteState == AH::Button::Pressed && strumUpState == AH::Button::Falling) {
      chordUp(chord, chordSize, octavePtr, velocityPtr);
    } else if (noteState == AH::Button::Rising) {
      chordOff(chord, chordSize, octavePtr, velocityPtr);
    }
  }

 private:
  AH::Button noteButton;
  AH::Button strumUpSwitch;
  AH::Button strumDownSwitch;
  int chord;
  uint8_t* chordSize;
  uint8_t* octavePtr;
  uint8_t* velocityPtr;
  uint8_t* strumSpeed;

  void chordDown (int* chord, uint8_t* chordSize, uint8_t* octavePtr, uint8_t* velocityPtr) {
    int i = 0;

    while (i< *chordSize) {
      Control_Surface.sendNoteOn({chord[i], Channel_1}, *velocityPtr);
      delay(*strumSpeed);
      i++;
    }
  }

  void chordUp (int* chord, uint8_t* chordSize, uint8_t* octavePtr, uint8_t* velocityPtr) {
    int i = (*chordSize) - 1;
    while (i > -1) {
      Control_Surface.sendNoteOn({chord[i], Channel_1}, *velocityPtr);
      delay(*strumSpeed);
      i--;
    }
  }

  void chordOff (int* chord, uint8_t* chordSize, uint8_t* octavePtr, uint8_t* velocityPtr) {
    int i = 0;
    while (i < *chordSize) {
      Control_Surface.sendNoteOff({chord[i], Channel_1}, *velocityPtr);
      i++;
    }
  }

};

class MyHighNoteButton : public MIDIOutputElement {
 public:
  MyHighNoteButton(pin_t notePin, pin_t strumUpPin, pin_t strumDownPin, MIDI_Notes::Note note, uint8_t* octavePtr, uint8_t* velocityPtr)
    : noteButton(notePin), strumUpSwitch(strumUpPin), strumDownSwitch(strumDownPin), note(note), octavePtr(octavePtr), velocityPtr(velocityPtr) {}

  void begin() final override { 
    noteButton.begin(); 
    strumUpSwitch.begin();
    strumDownSwitch.begin();
  }

  void update() final override {
    AH::Button::State noteState = noteButton.update();
    AH::Button::State strumUpState = strumUpSwitch.update();
    AH::Button::State strumDownState = strumDownSwitch.update();

    if (noteState == AH::Button::Pressed && strumDownState == AH::Button::Falling) {
      Control_Surface.sendNoteOn({note[*octavePtr], Channel_1}, *velocityPtr);  // Use the current velocity value
    } else if (noteState == AH::Button::Pressed && strumUpState == AH::Button::Falling) {
      Control_Surface.sendNoteOn({note[*octavePtr], Channel_1}, *velocityPtr);  // Use the current velocity value
    } else if (noteState == AH::Button::Rising) {
      Control_Surface.sendNoteOff({note[*octavePtr], Channel_1}, *velocityPtr); // Use the current velocity value
    }
  }

 private:
  AH::Button noteButton;
  AH::Button strumUpSwitch;
  AH::Button strumDownSwitch;
  MIDI_Notes::Note note;
  uint8_t* octavePtr;
  uint8_t* velocityPtr;  // Pointer to velocity value
};

// Pin definitions
int strumUpPin = 2;
int strumDownPin = 3;

int upSelect = 5;
int downSelect = 6;
int leftSelect = 4;
int rightSelect = 7;

int greenLowButton = 40;
int redLowButton = 42;
int yellowLowButton = 44;
int blueLowButton = 46;
int orangeLowButton = 48;

int greenHighButton = 41;
int redHighButton = 43;
int yellowHighButton = 45;
int blueHighButton = 47;
int orangeHighButton = 49;

int octaveUp = 9;
int octaveDown = 8;

int okButton = 10;
int backButton = 11;

int fiveSelectSwitch = A1;

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

uint8_t vel = 127;
uint8_t octave = 4;
uint8_t strumSpeed = 25;

int ppp = 16;
int pp = 32;
int p = 48;
int mp = 64;
int mf = 80;
int f = 96;
int ff = 112;
int fff = 127;

bool chordChanged = false;

// C major 1st Pos
int cMaj1[] = {MIDI_Notes::C[3], MIDI_Notes::E[3], MIDI_Notes::G[3], MIDI_Notes::C[4], MIDI_Notes::E[4]}; //size=5

// C#/Db Major 1st Pos
int cSharpMaj1[] = {MIDI_Notes::Db[3], MIDI_Notes::F[3], MIDI_Notes::Ab[3], MIDI_Notes::Db[4], MIDI_Notes::F[4]}; //size=5

// D Major 1st Pos 
int dMaj1[] = {MIDI_Notes::D[3], MIDI_Notes::A[3], MIDI_Notes::D[4], MIDI_Notes::Gb[4]}; //size=4

// D#/Eb Major 1st Pos
int dSharpMaj1[] = {MIDI_Notes::Eb[3], MIDI_Notes::Bb[3], MIDI_Notes::Eb[4], MIDI_Notes::G[4]}; //size=4

// E Major 1st Pos
int eMaj1[] = {MIDI_Notes::E[2], MIDI_Notes::B[2], MIDI_Notes::E[3], MIDI_Notes::Ab[3], MIDI_Notes::B[3], MIDI_Notes::E[4]}; //size=6

// F Major 1st Pos
int fMaj1[] = {MIDI_Notes::F[2], MIDI_Notes::C[3], MIDI_Notes::F[3], MIDI_Notes::A[3], MIDI_Notes::C[4], MIDI_Notes::F[4]}; //size=6

// F#/Gb Major 1st Pos
int fSharpMaj1[] = {MIDI_Notes::Gb[2], MIDI_Notes::Db[3], MIDI_Notes::Gb[3], MIDI_Notes::Bb[3], MIDI_Notes::Db[4], MIDI_Notes::Gb[4]}; //size=6

// G major 1st Pos
int gMaj1[] = {MIDI_Notes::G[2], MIDI_Notes::B[2], MIDI_Notes::D[3], MIDI_Notes::G[3], MIDI_Notes::B[3], MIDI_Notes::G[4]}; //size=6

// G#/Ab major 1st Pos
int gSharpMaj1[] = {MIDI_Notes::Eb[3], MIDI_Notes::Ab[3], MIDI_Notes::C[4], MIDI_Notes::Ab[4]}; //size=4

// A Major 1st Pos
int aMaj1[] = {MIDI_Notes::A[2], MIDI_Notes::E[3], MIDI_Notes::A[3], MIDI_Notes::Db[4], MIDI_Notes::E[4]}; //size=5

// A#/Bb Major 1st Pos
int aSharpMaj1[] = {MIDI_Notes::Bb[2], MIDI_Notes::F[3], MIDI_Notes::Bb[3], MIDI_Notes::D[4], MIDI_Notes::F[4]}; //size=5

// B Major 1st Pos
int bMaj1[] = {MIDI_Notes::B[2], MIDI_Notes::Gb[3], MIDI_Notes::B[3], MIDI_Notes::Eb[4], MIDI_Notes::Gb[4]}; //size5

// C Minor 1st Pos
int cMin1[] = {MIDI_Notes::C[3], MIDI_Notes::Eb[3], MIDI_Notes::G[3], MIDI_Notes::C[4], MIDI_Notes::G[4]};//size5

// C#/Db Minor 1st Pos
int cSharpMin1[] = {MIDI_Notes::Db[3], MIDI_Notes::Ab[3], MIDI_Notes::Db[4], MIDI_Notes::E[4], MIDI_Notes::Ab[4]};//size=5

// D Minor 1st Pos
int dMin1[] = {MIDI_Notes::D[3], MIDI_Notes::A[3], MIDI_Notes::D[4], MIDI_Notes::F[4]};//size=4

// D#/Eb Minor 1st Pos
int dSharpMin1[] = {MIDI_Notes::Eb[3], MIDI_Notes::Bb[3], MIDI_Notes::Eb[4], MIDI_Notes::Gb[4]};//size=4

// E Minor 1st Pos
int eMin1[] = {MIDI_Notes::E[2], MIDI_Notes::B[2], MIDI_Notes::E[3], MIDI_Notes::G[3], MIDI_Notes::B[3], MIDI_Notes::E[4]};//size=6

// F Minor 1st Pos
int fMin1[] = {MIDI_Notes::F[2], MIDI_Notes::C[3], MIDI_Notes::F[3], MIDI_Notes::Ab[3], MIDI_Notes::C[4], MIDI_Notes::F[4]};//size=6

// F#/Gb Minor 1st Pos
int fSharpMin1[] = {MIDI_Notes::Gb[2], MIDI_Notes::Db[3], MIDI_Notes::Gb[3], MIDI_Notes::A[3], MIDI_Notes::Db[4], MIDI_Notes::Gb[4]};//size=6

// G Minor 1st Pos
int gMin1[] = {MIDI_Notes::G[2], MIDI_Notes::D[3], MIDI_Notes::G[3], MIDI_Notes::Bb[3], MIDI_Notes::D[4], MIDI_Notes::G[4]};//size=6

// G#/Ab Minor 1st Pos
int gSharpMin1[] = {MIDI_Notes::Ab[2], MIDI_Notes::Eb[3], MIDI_Notes::Ab[3], MIDI_Notes::B[3], MIDI_Notes::Eb[4], MIDI_Notes::Ab[4]};//size=6

// A Minor 1st Pos
int aMin1[] = {MIDI_Notes::A[2], MIDI_Notes::E[3], MIDI_Notes::A[3], MIDI_Notes::C[4], MIDI_Notes::E[4]};//size=5

// A#/Bb Minor 1st Pos
int aSharpMin1[] = {MIDI_Notes::Bb[2], MIDI_Notes::F[3], MIDI_Notes::Bb[3], MIDI_Notes::Db[4], MIDI_Notes::F[4]};//size=5

// B Minor 1st Pos
int bMin1[] = {MIDI_Notes::B[2], MIDI_Notes::Gb[3], MIDI_Notes::B[3], MIDI_Notes::D[4], MIDI_Notes::Gb[4]};//size=5

// Array of chord pointers for greenChord
int* chords[] = {cMaj1, cSharpMaj1, dMaj1, dSharpMaj1, eMaj1, fMaj1, fSharpMaj1, gMaj1, gSharpMaj1, aMaj1, aSharpMaj1, bMaj1, cMin1, cSharpMin1, dMin1, dSharpMin1, eMin1, fMin1, fSharpMin1, gMin1, gSharpMin1, aMin1, aSharpMin1, bMin1};

uint8_t chordSizes[] = {
    sizeof(cMaj1) / sizeof(int),        // Size of C Major 1st Position
    sizeof(cSharpMaj1) / sizeof(int),   // Size of C#/Db Major 1st Position
    sizeof(dMaj1) / sizeof(int),        // Size of D Major 1st Position
    sizeof(dSharpMaj1) / sizeof(int),   // Size of D#/Eb Major 1st Position
    sizeof(eMaj1) / sizeof(int),        // Size of E Major 1st Position
    sizeof(fMaj1) / sizeof(int),        // Size of F Major 1st Position
    sizeof(fSharpMaj1) / sizeof(int),   // Size of F#/Gb Major 1st Position
    sizeof(gMaj1) / sizeof(int),        // Size of G Major 1st Position
    sizeof(gSharpMaj1) / sizeof(int),   // Size of G#/Ab Major 1st Position
    sizeof(aMaj1) / sizeof(int),        // Size of A Major 1st Position
    sizeof(aSharpMaj1) / sizeof(int),   // Size of A#/Bb Major 1st Position
    sizeof(bMaj1) / sizeof(int),        // Size of B Major 1st Position
    sizeof(cMin1) / sizeof(int),        // Size of C Major 1st Position
    sizeof(cSharpMin1) / sizeof(int),   // Size of C#/Db Major 1st Position
    sizeof(dMin1) / sizeof(int),        // Size of D Major 1st Position
    sizeof(dSharpMin1) / sizeof(int),   // Size of D#/Eb Major 1st Position
    sizeof(eMin1) / sizeof(int),        // Size of E Major 1st Position
    sizeof(fMin1) / sizeof(int),        // Size of F Major 1st Position
    sizeof(fSharpMin1) / sizeof(int),   // Size of F#/Gb Major 1st Position
    sizeof(gMin1) / sizeof(int),        // Size of G Major 1st Position
    sizeof(gSharpMin1) / sizeof(int),   // Size of G#/Ab Major 1st Position
    sizeof(aMin1) / sizeof(int),        // Size of A Major 1st Position
    sizeof(aSharpMin1) / sizeof(int),   // Size of A#/Bb Major 1st Position
    sizeof(bMin1) / sizeof(int)         // Size of B Major 1st Position
};

const char* chordNames[] = {
  "C",
  "C#",
  "D",
  "D#",
  "E",
  "F",
  "F#",
  "G",
  "G#",
  "A",
  "A#",
  "B",
  "c",
  "c#",
  "d",
  "d#",
  "e",
  "f",
  "f#",
  "g",
  "g#",
  "a",
  "a#",
  "b",
};

//Defaults upon initialization
uint8_t currentChordEditMode = 0;  // 0 = Green, 1 = Red, 2 = Yellow, 3 = Blue, 4 = Orange
uint8_t numChords = 24;

uint8_t currentGreenChordIndex = 0; //C
uint8_t currentRedChordIndex = 7; //G
uint8_t currentYellowChordIndex = 22; //Am
uint8_t currentBlueChordIndex = 5; //F
uint8_t currentOrangeChordIndex = 11; //Bb

int greenChord[MAX_CHORD_SIZE];
int redChord[MAX_CHORD_SIZE];
int yellowChord[MAX_CHORD_SIZE];
int blueChord[MAX_CHORD_SIZE];
int orangeChord[MAX_CHORD_SIZE];

uint8_t greenChordSize = sizeof(greenChord) / sizeof(int);
uint8_t redChordSize = sizeof(redChord) / sizeof(int);
uint8_t yellowChordSize = sizeof(yellowChord) / sizeof(int);
uint8_t blueChordSize = sizeof(blueChord) / sizeof(int);
uint8_t orangeChordSize = sizeof(orangeChord) / sizeof(int);

// Low (Top) Buttons for chords
MyChordButton greenLow {greenLowButton, strumUpPin, strumDownPin, greenChord, &greenChordSize, &octave, &vel, &strumSpeed}; // 60 is MIDI note for C4
MyChordButton redLow {redLowButton, strumUpPin, strumDownPin, redChord, &redChordSize, &octave, &vel, &strumSpeed}; // 62 is MIDI note for D4
MyChordButton yellowLow {yellowLowButton, strumUpPin, strumDownPin, yellowChord, &yellowChordSize, &octave, &vel, &strumSpeed}; // 64 is MIDI note for E4
MyChordButton blueLow {blueLowButton, strumUpPin, strumDownPin, blueChord, &blueChordSize, &octave, &vel, &strumSpeed}; // 65 is MIDI note for F4
MyChordButton orangeLow {orangeLowButton, strumUpPin, strumDownPin, orangeChord, &orangeChordSize, &octave, &vel, &strumSpeed}; // 67 is MIDI note for G4

// High (Bottom) Buttons for single notes
MyHighNoteButton greenHigh {greenHighButton, strumUpPin, strumDownPin, MIDI_Notes::C, &octave, &vel};
MyHighNoteButton redHigh {redHighButton, strumUpPin, strumDownPin, MIDI_Notes::G, &octave, &vel};
MyHighNoteButton yellowHigh {yellowHighButton, strumUpPin, strumDownPin, MIDI_Notes::A, &octave, &vel};
MyHighNoteButton blueHigh {blueHighButton, strumUpPin, strumDownPin, MIDI_Notes::F, &octave, &vel};
MyHighNoteButton orangeHigh {orangeHighButton, strumUpPin, strumDownPin, MIDI_Notes::Bb, &octave, &vel};

// "Whammy" Bar
PBPotentiometer pitchBend {
  A0,
  Channel_1
};

LiquidCrystal lcd(lcdRS, lcdE, lcdD0, lcdD1, lcdD2, lcdD3, lcdD4, lcdD5, lcdD6, lcdD7);

uint8_t getVelocityFromAnalogValue(int analogValue) {
  if (analogValue >= 0 && analogValue <= 200) {
    return ppp; 
  } else if (analogValue >= 200 && analogValue <= 365) {
    return p;
  } else if (analogValue >= 400 && analogValue <= 585) {
    return mf;
  } else if (analogValue >= 620 && analogValue <= 700) {
    return f;
  } else if (analogValue >= 701 && analogValue <= 1023) {
    return fff;
  } else {
    return mf;
  }
}

void setup() {
  pinMode(octaveUp, INPUT_PULLUP);
  pinMode(octaveDown, INPUT_PULLUP);

  pinMode(upSelect, INPUT_PULLUP);
  pinMode(downSelect, INPUT_PULLUP);
  pinMode(leftSelect, INPUT_PULLUP);
  pinMode(rightSelect, INPUT_PULLUP);
  pinMode(okButton, INPUT_PULLUP);
  pinMode(backButton, INPUT_PULLUP);

  memcpy(greenChord, cMaj1, sizeof(cMaj1));
  memcpy(redChord, gMaj1, sizeof(gMaj1));
  memcpy(yellowChord, aMin1, sizeof(aMin1));
  memcpy(blueChord, fMaj1, sizeof(fMaj1));
  memcpy(orangeChord, aSharpMaj1, sizeof(aSharpMaj1));

  // Set up the LCD's number of columns and rows
  lcd.begin(16, 2);
  Control_Surface.begin();
}

void loop() {
  Control_Surface.loop();

    if (chordChanged) {
    lcd.clear();
  }

  chordChanged = false;

  // Print the top line of the 16x2 LCD
  lcd.setCursor(0, 0);
  lcd.print("Strum Speed: ");
  lcd.print(strumSpeed);

  // Print the botto line of 16x2 LCD
  lcd.setCursor(0, 1);
  switch (currentChordEditMode) {
    case 0:
      lcd.print("Green: ");
      lcd.print(chordNames[currentGreenChordIndex]);
      break;
    case 1:
      lcd.print("Red: ");
      lcd.print(chordNames[currentRedChordIndex]);
      break;
    case 2:
      lcd.print("Yellow: ");
      lcd.print(chordNames[currentYellowChordIndex]);
      break;
    case 3:
      lcd.print("Blue: ");
      lcd.print(chordNames[currentBlueChordIndex]);
      break;
    case 4:
      lcd.print("Orange: ");
      lcd.print(chordNames[currentOrangeChordIndex]);
      break;
  }

  // Read the value from the potentiometer
  int fiveSelectSwitchVal = analogRead(fiveSelectSwitch);
  vel = getVelocityFromAnalogValue(fiveSelectSwitchVal);

  // Update octave based on button presses
  if (digitalRead(octaveUp) == LOW) {
    if (octave < 8) octave++;
    delay(200); // debounce delay
  }
  
  if (digitalRead(octaveDown) == LOW) {
    if (octave > -2) octave--;
    delay(200); // debounce zq
  }

if (digitalRead(downSelect) == LOW) {
    switch (currentChordEditMode) {
      case 0:
        currentGreenChordIndex = (currentGreenChordIndex > 0) ? currentGreenChordIndex - 1 : numChords - 1;
        memcpy(greenChord, chords[currentGreenChordIndex], chordSizes[currentGreenChordIndex] * sizeof(int));
        greenChordSize = chordSizes[currentGreenChordIndex];
        break;
      case 1:
        currentRedChordIndex = (currentRedChordIndex > 0) ? currentRedChordIndex - 1 : numChords - 1;
        memcpy(redChord, chords[currentRedChordIndex], chordSizes[currentRedChordIndex] * sizeof(int));
        redChordSize = chordSizes[currentRedChordIndex];
        break;
      case 2:
        currentYellowChordIndex = (currentYellowChordIndex > 0) ? currentYellowChordIndex - 1 : numChords - 1;
        memcpy(yellowChord, chords[currentYellowChordIndex], chordSizes[currentYellowChordIndex] * sizeof(int));
        yellowChordSize = chordSizes[currentYellowChordIndex];
        break;
      case 3:
        currentBlueChordIndex = (currentBlueChordIndex > 0) ? currentBlueChordIndex - 1 : numChords - 1;
        memcpy(blueChord, chords[currentBlueChordIndex], chordSizes[currentBlueChordIndex] * sizeof(int));
        blueChordSize = chordSizes[currentBlueChordIndex];
        break;
      case 4:
        currentOrangeChordIndex = (currentOrangeChordIndex > 0) ? currentOrangeChordIndex - 1 : numChords - 1;
        memcpy(orangeChord, chords[currentOrangeChordIndex], chordSizes[currentOrangeChordIndex] * sizeof(int));
        orangeChordSize = chordSizes[currentOrangeChordIndex];
        break;
    }
    chordChanged = true;
    delay(200);  // Debounce delay
}

if (digitalRead(upSelect) == LOW) {
    switch (currentChordEditMode) {
      case 0:
        currentGreenChordIndex = (currentGreenChordIndex + 1) % numChords;
        memcpy(greenChord, chords[currentGreenChordIndex], chordSizes[currentGreenChordIndex] * sizeof(int));
        greenChordSize = chordSizes[currentGreenChordIndex];
        break;
      case 1:
        currentRedChordIndex = (currentRedChordIndex + 1) % numChords;
        memcpy(redChord, chords[currentRedChordIndex], chordSizes[currentRedChordIndex] * sizeof(int));
        redChordSize = chordSizes[currentRedChordIndex];
        break;
      case 2:
        currentYellowChordIndex = (currentYellowChordIndex + 1) % numChords;
        memcpy(yellowChord, chords[currentYellowChordIndex], chordSizes[currentYellowChordIndex] * sizeof(int));
        yellowChordSize = chordSizes[currentYellowChordIndex];
        break;
      case 3:
        currentBlueChordIndex = (currentBlueChordIndex + 1) % numChords;
        memcpy(blueChord, chords[currentBlueChordIndex], chordSizes[currentBlueChordIndex] * sizeof(int));
        blueChordSize = chordSizes[currentBlueChordIndex];
        break;
      case 4:
        currentOrangeChordIndex = (currentOrangeChordIndex + 1) % numChords;
        memcpy(orangeChord, chords[currentOrangeChordIndex], chordSizes[currentOrangeChordIndex] * sizeof(int));
        orangeChordSize = chordSizes[currentOrangeChordIndex];
        break;
    }
    chordChanged = true;
    delay(200);  // Debounce delay
}
  
  if (digitalRead(leftSelect) == LOW) {
    Serial.println("Left button pressed");
    delay(200); // debounce delay
    lcd.clear();
    strumSpeed = strumSpeed - 5;
  }
  
  if (digitalRead(rightSelect) == LOW) {
    Serial.println("Right button pressed");
    delay(200); // debounce delay
    lcd.clear();
    strumSpeed = strumSpeed + 5;
  }

  // Handle okButton press to move to the next chord
  if (digitalRead(okButton) == LOW) {
    currentChordEditMode = (currentChordEditMode + 1) % 5;  // Cycle through 0 to 4
    delay(200); // Debounce delay
  }

  // Handle backButton press to move to the previous chord
  if (digitalRead(backButton) == LOW) {
    currentChordEditMode = (currentChordEditMode == 0) ? 4 : currentChordEditMode - 1;  // Cycle backwards
    delay(200); // Debounce delay
  }
}
