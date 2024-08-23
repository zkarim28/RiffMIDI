#include <Control_Surface.h>
#include <MIDI_Constants/Chords/Chords.hpp>
#include <LiquidCrystal.h>

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

  //   void chordDown (int* chord, uint8_t* octavePtr, uint8_t* velocityPtr) {
  //     Control_Surface.sendNoteOn({chord[0], Channel_1}, *velocityPtr);
  //     delay(strumSpeed);
  //     Control_Surface.sendNoteOn({chord[1], Channel_1}, *velocityPtr);
  //     delay(strumSpeed);
  //     Control_Surface.sendNoteOn({chord[2], Channel_1}, *velocityPtr);
  //     delay(strumSpeed);
  // }

  void chordUp (int* chord, uint8_t* chordSize, uint8_t* octavePtr, uint8_t* velocityPtr) {
    int i = (*chordSize) - 1;
    while (i > -1) {
      Control_Surface.sendNoteOn({chord[i], Channel_1}, *velocityPtr);
      delay(*strumSpeed);
      i--;
    }
  }
  
  // void chordUp (int* chord, uint8_t* octavePtr, uint8_t* velocityPtr) {
  //     Control_Surface.sendNoteOn({chord[2], Channel_1}, *velocityPtr);
  //     delay(strumSpeed);
  //     Control_Surface.sendNoteOn({chord[1], Channel_1}, *velocityPtr);
  //     delay(strumSpeed);
  //     Control_Surface.sendNoteOn({chord[0], Channel_1}, *velocityPtr);
  //     delay(strumSpeed);
  // }

  void chordOff (int* chord, uint8_t* chordSize, uint8_t* octavePtr, uint8_t* velocityPtr) {
    int i = 0;
    while (i < *chordSize) {
      Control_Surface.sendNoteOff({chord[i], Channel_1}, *velocityPtr);
      i++;
    }
  }

  // void chordOff (int* chord, uint8_t* octavePtr, uint8_t* velocityPtr) {
  //     Control_Surface.sendNoteOff({chord[0], Channel_1}, *velocityPtr);
  //     Control_Surface.sendNoteOff({chord[1], Channel_1}, *velocityPtr);
  //     Control_Surface.sendNoteOff({chord[2], Channel_1}, *velocityPtr);
  // }

};

class MyHighNoteButton : public MIDIOutputElement {
 public:
  MyHighNoteButton(pin_t notePin, MIDI_Notes::Note note, uint8_t* octavePtr, uint8_t* velocityPtr)
    : noteButton(notePin), note(note), octavePtr(octavePtr), velocityPtr(velocityPtr) {}

  void begin() final override { 
    noteButton.begin(); 
  }

  void update() final override {
    AH::Button::State noteState = noteButton.update();

    if (noteState == AH::Button::Falling) {
      Control_Surface.sendNoteOn({note[*octavePtr], Channel_1}, *velocityPtr);  // Use the current velocity value
    } else if (noteState == AH::Button::Rising) {
      Control_Surface.sendNoteOff({note[*octavePtr], Channel_1}, *velocityPtr); // Use the current velocity value
    }
  }

 private:
  AH::Button noteButton;
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
uint8_t strumSpeed = 40;

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
int cMaj1[] = {MIDI_Notes::C[3], MIDI_Notes::E[3], MIDI_Notes::G[3], MIDI_Notes::C[4], MIDI_Notes::E[4]}; //5

// C#/Db Major 1st Pos
int cSharpMaj1[] = {MIDI_Notes::Db[3], MIDI_Notes::F[3], MIDI_Notes::Ab[3], MIDI_Notes::Db[4], MIDI_Notes::F[4]}; //5
//int dFlatMaj1[]

// D Major 1st Pos 
int dMaj1[] = {MIDI_Notes::D[3], MIDI_Notes::A[3], MIDI_Notes::D[4], MIDI_Notes::Gb[4]}; //4

// D#/Eb Major 1st Pos
int dSharpMaj1[] = {MIDI_Notes::Eb[3], MIDI_Notes::Bb[3], MIDI_Notes::Eb[4], MIDI_Notes::G[4]}; //4
//int eFlatMaj1[]

// E Major 1st Pos
int eMaj1[] = {MIDI_Notes::E[2], MIDI_Notes::B[2], MIDI_Notes::E[3], MIDI_Notes::Ab[3], MIDI_Notes::B[3], MIDI_Notes::E[4]}; //6

// F Major 1st Pos
int fMaj1[] = {MIDI_Notes::F[2], MIDI_Notes::C[3], MIDI_Notes::F[3], MIDI_Notes::A[3], MIDI_Notes::C[4], MIDI_Notes::F[4]}; //6

// F#/Gb Major 1st Pos
int fSharpMaj1[] = {MIDI_Notes::Gb[2], MIDI_Notes::Db[3], MIDI_Notes::Gb[3], MIDI_Notes::Bb[3], MIDI_Notes::Db[4], MIDI_Notes::Gb[4]}; //6
// int gFlatMaj1[]

// G major 1st Pos
int gMaj1[] = {MIDI_Notes::G[2], MIDI_Notes::B[2], MIDI_Notes::D[3], MIDI_Notes::G[3], MIDI_Notes::B[3], MIDI_Notes::G[4]}; //6

// G#/Ab major 1st Pos
int gSharpMaj1[] = {MIDI_Notes::Eb[3], MIDI_Notes::Ab[3], MIDI_Notes::C[4], MIDI_Notes::Ab[4]}; //4
//int aFlatMaj1[]

// A Major 1st Pos
int aMaj1[] = {MIDI_Notes::A[2], MIDI_Notes::E[3], MIDI_Notes::A[3], MIDI_Notes::Db[4], MIDI_Notes::E[4]}; //5

// A#/Bb Major 1st Pos
int aSharpMaj1[] = {MIDI_Notes::Bb[2], MIDI_Notes::F[3], MIDI_Notes::Bb[3], MIDI_Notes::D[4], MIDI_Notes::F[4]}; //5
//int bFlatMaj1

//B Major 1st Pos
int bMaj1[] = {MIDI_Notes::B[2], MIDI_Notes::Gb[3], MIDI_Notes::B[3], MIDI_Notes::Eb[4], MIDI_Notes::Gb[4]}; //5

// Declare greenChord with the same size as cMaj1
int greenChord[sizeof(cMaj1) / sizeof(int)];

// Array of chord pointers for greenChord
int* greenChords[] = {cMaj1, cSharpMaj1, dMaj1, dSharpMaj1, eMaj1, fMaj1, fSharpMaj1, gMaj1, gSharpMaj1, aMaj1, aSharpMaj1, bMaj1};

uint8_t greenChordSizes[] = {
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
    sizeof(bMaj1) / sizeof(int)         // Size of B Major 1st Position
};

const char* greenChordNames[] = {
  "C Major",
  "C# Major",
  "D Major",
  "D# Major",
  "E Major",
  "F Major",
  "F# Major",
  "G Major",
  "G# Major",
  "A Major",
  "A# Major",
  "B Major"
};

uint8_t numGreenChords = 12;
uint8_t currentGreenChordIndex = 0;

int redChord[] = {MIDI_Notes::A[2], MIDI_Notes::E[3], MIDI_Notes::A[3], MIDI_Notes::C[4], MIDI_Notes::E[4]}; // A minor 1st Pos
int yellowChord[] = {MIDI_Notes::G[2], MIDI_Notes::B[2], MIDI_Notes::D[3], MIDI_Notes::G[3], MIDI_Notes::B[3], MIDI_Notes::G[4]}; // G major 1st Pos
int blueChord[] = {MIDI_Notes::E[2], MIDI_Notes::B[2], MIDI_Notes::E[3], MIDI_Notes::G[3], MIDI_Notes::B[3], MIDI_Notes::E[4]}; // E minor 1st Pos
int orangeChord[] = {MIDI_Notes::F[2], MIDI_Notes::C[3], MIDI_Notes::F[3], MIDI_Notes::A[3], MIDI_Notes::C[4], MIDI_Notes::F[4]}; // F minor 1st Pos

uint8_t greenChordSize = sizeof(greenChord) / sizeof(int);
uint8_t redChordSize = sizeof(redChord) / sizeof(int);
uint8_t yellowChordSize = sizeof(yellowChord) / sizeof(int);
uint8_t blueChordSize = sizeof(blueChord) / sizeof(int);
uint8_t orangeChordSize = sizeof(orangeChord) / sizeof(int);
String lastChordPlayed = "None";

// Low Buttons for chords
MyChordButton greenLow {greenLowButton, strumUpPin, strumDownPin, greenChord, &greenChordSize, &octave, &vel, &strumSpeed}; // 60 is MIDI note for C4
MyChordButton redLow {redLowButton, strumUpPin, strumDownPin, redChord, &redChordSize, &octave, &vel, &strumSpeed}; // 62 is MIDI note for D4
MyChordButton yellowLow {yellowLowButton, strumUpPin, strumDownPin, yellowChord, &yellowChordSize, &octave, &vel, &strumSpeed}; // 64 is MIDI note for E4
MyChordButton blueLow {blueLowButton, strumUpPin, strumDownPin, blueChord, &blueChordSize, &octave, &vel, &strumSpeed}; // 65 is MIDI note for F4
MyChordButton orangeLow {orangeLowButton, strumUpPin, strumDownPin, orangeChord, &orangeChordSize, &octave, &vel, &strumSpeed}; // 67 is MIDI note for G4

// High Buttons for single notes
MyHighNoteButton greenHigh {greenHighButton, MIDI_Notes::C, &octave, &vel};
MyHighNoteButton redHigh {redHighButton, MIDI_Notes::D, &octave, &vel};
MyHighNoteButton yellowHigh {yellowHighButton, MIDI_Notes::F, &octave, &vel};
MyHighNoteButton blueHigh {blueHighButton, MIDI_Notes::Ab, &octave, &vel};
MyHighNoteButton orangeHigh {orangeHighButton, MIDI_Notes::Bb, &octave, &vel};

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

  // Serial.begin(9600); // Initialize serial communication

  // Set up the LCD's number of columns and rows
  lcd.begin(16, 2);
  Control_Surface.begin();
}

void loop() {
  Control_Surface.loop();

  chordChanged = false;

  // Print the first line
  lcd.setCursor(0, 0);
  lcd.print("Strum Speed: ");
  lcd.print(strumSpeed);

  // Print the second line
  lcd.setCursor(0, 1);
  lcd.print("Chord: ");
  lcd.print(greenChordNames[currentGreenChordIndex]);

  // Read the value from the potentiometer
  int fiveSelectSwitchVal = analogRead(fiveSelectSwitch);
  vel = getVelocityFromAnalogValue(fiveSelectSwitchVal);

  // Serial.println(fiveSelectSwitchVal);

      // Update the LCD only if the chord has changed
    if (chordChanged) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("");
        lcd.setCursor(0, 1);
        lcd.print(greenChordNames[currentGreenChordIndex]);
    }

  // Update octave based on button presses
  if (digitalRead(octaveUp) == LOW) {
    if (octave < 8) octave++;
    delay(200); // debounce delay
  }
  
  if (digitalRead(octaveDown) == LOW) {
    if (octave > -2) octave--;
    delay(200); // debounce zq
  }

  if (digitalRead(upSelect) == LOW) {
    currentGreenChordIndex = (currentGreenChordIndex + 1) % numGreenChords;  // Increment and wrap around
    memset(greenChord, 0, sizeof(greenChord)); // Clear old values in greenChord
    memcpy(greenChord, greenChords[currentGreenChordIndex], greenChordSizes[currentGreenChordIndex] * sizeof(int));  // Copy the new chord into greenChord
    greenChordSize = greenChordSizes[currentGreenChordIndex]; // Update the size of the current chord
    chordChanged = true;
    delay(200);  // Debounce delay
  }
    
  if (digitalRead(downSelect) == LOW) {
    currentGreenChordIndex = (currentGreenChordIndex > 0) ? currentGreenChordIndex - 1 : numGreenChords - 1;  // Decrement and wrap around
    memset(greenChord, 0, sizeof(greenChord)); // Clear old values in greenChord
    memcpy(greenChord, greenChords[currentGreenChordIndex], greenChordSizes[currentGreenChordIndex] * sizeof(int));  // Copy the new chord into greenChord
    greenChordSize = greenChordSizes[currentGreenChordIndex]; // Update the size of the current chord
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

  if (digitalRead(okButton) == LOW) {
    Serial.println("OK button pressed");
    delay(200); // debounce delay
    lcd.clear();
    strumSpeed = strumSpeed + 5;
  }

  if (digitalRead(backButton) == LOW) {
    Serial.println("Back button pressed");
    delay(200); // debounce delay
    lcd.clear();
    strumSpeed = strumSpeed + 5;
  }
}