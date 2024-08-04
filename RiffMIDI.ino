#include <Control_Surface.h>
#include <MIDI_Constants/Chords/Chords.hpp>

USBMIDI_Interface midi;

class MyNoteButton : public MIDIOutputElement {
 public:
  MyNoteButton(pin_t notePin, pin_t strumUpPin, pin_t strumDownPin, MIDI_Notes::Note* chord, uint8_t* octavePtr, uint8_t* velocityPtr)
    : noteButton(notePin), strumUpSwitch(strumUpPin), strumDownSwitch(strumDownPin), chord(chord), octavePtr(octavePtr), velocityPtr(velocityPtr) {}

  void begin() final override { 
    noteButton.begin(); 
    strumUpSwitch.begin();
    strumDownSwitch.begin();
  }

  void update() final override {
    AH::Button::State noteState = noteButton.update();
    AH::Button::State strumUpState = strumUpSwitch.update();
    AH::Button::State strumDownState = strumDownSwitch.update();

    if (noteState == AH::Button::Pressed && (strumUpState == AH::Button::Falling || strumDownState == AH::Button::Falling)) {
      // Control_Surface.sendNoteOn({note[*octavePtr], Channel_1}, *velocityPtr);  // Use the current velocity value

      Control_Surface.sendNoteOn({chord[0][*octavePtr], Channel_1}, *velocityPtr);
      delay(strumSpeed);
      Control_Surface.sendNoteOn({chord[1][*octavePtr], Channel_1}, *velocityPtr);
      delay(strumSpeed);
      Control_Surface.sendNoteOn({chord[2][*octavePtr], Channel_1}, *velocityPtr);
      delay(strumSpeed);

    } else if (noteState == AH::Button::Rising) {
      Control_Surface.sendNoteOff({chord[0][*octavePtr], Channel_1}, *velocityPtr);
      delay(strumSpeed);
      Control_Surface.sendNoteOff({chord[1][*octavePtr], Channel_1}, *velocityPtr);
      delay(strumSpeed);
      Control_Surface.sendNoteOff({chord[2][*octavePtr], Channel_1}, *velocityPtr);
      delay(strumSpeed);
    }
  }

 private:
  AH::Button noteButton;
  AH::Button strumUpSwitch;
  AH::Button strumDownSwitch;
  MIDI_Notes::Note* chord;
  uint8_t* octavePtr;
  uint8_t* velocityPtr;  // Pointer to velocity value
  int strumSpeed = 50;

  void chordUp () {
    
  }

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
int strumDownPin = 2;
int strumUpPin = 3;

int upSelect = 4;
int downSelect = 5;
int leftSelect = 6;
int rightSelect = 7;

int greenLowButton = 41;
int redLowButton = 43;
int yellowLowButton = 45;
int blueLowButton = 47;
int orangeLowButton = 49;

int greenHighButton = 40;
int redHighButton = 42;
int yellowHighButton = 44;
int blueHighButton = 46;
int orangeHighButton = 48;

int octaveUp = 8;
int octaveDown = 9;

int okButton = 10;
int backButton = 11;

int fiveSelectSwitch = A1;

uint8_t vel = 127;
uint8_t octave = 4;

int ppp = 16;
int pp = 32;
int p = 48;
int mp = 64;
int mf = 80;
int f = 96;
int ff = 112;
int fff = 127;

//Arrays of MIDI_Notes to create chords
MIDI_Notes::Note greenChord[] = {MIDI_Notes::C, MIDI_Notes::E, MIDI_Notes::G}; // C major
MIDI_Notes::Note redChord[] = {MIDI_Notes::A, MIDI_Notes::C, MIDI_Notes::E}; // A minor
MIDI_Notes::Note yellowChord[] = {MIDI_Notes::G, MIDI_Notes::B, MIDI_Notes::D}; // G major
MIDI_Notes::Note blueChord[] = {MIDI_Notes::E, MIDI_Notes::G, MIDI_Notes::B}; // E minor
MIDI_Notes::Note orangeChord[] = {MIDI_Notes::D, MIDI_Notes::F, MIDI_Notes::A}; // D minor

// // Low Buttons for chords
// MyNoteButton greenLow {greenLowButton, strumUpPin, strumDownPin, MIDI_Notes::C, &octave, &vel}; // 60 is MIDI note for C4
// MyNoteButton redLow {redLowButton, strumUpPin, strumDownPin, MIDI_Notes::E, &octave, &vel}; // 62 is MIDI note for D4
// MyNoteButton yellowLow {yellowLowButton, strumUpPin, strumDownPin, MIDI_Notes::G, &octave, &vel}; // 64 is MIDI note for E4
// MyNoteButton blueLow {blueLowButton, strumUpPin, strumDownPin, MIDI_Notes::A, &octave, &vel}; // 65 is MIDI note for F4
// MyNoteButton orangeLow {orangeLowButton, strumUpPin, strumDownPin, MIDI_Notes::B, &octave, &vel}; // 67 is MIDI note for G4

// Low Buttons for chords
MyNoteButton greenLow {greenLowButton, strumUpPin, strumDownPin, greenChord, &octave, &vel}; // 60 is MIDI note for C4
MyNoteButton redLow {redLowButton, strumUpPin, strumDownPin, redChord, &octave, &vel}; // 62 is MIDI note for D4
MyNoteButton yellowLow {yellowLowButton, strumUpPin, strumDownPin, yellowChord, &octave, &vel}; // 64 is MIDI note for E4
MyNoteButton blueLow {blueLowButton, strumUpPin, strumDownPin, blueChord, &octave, &vel}; // 65 is MIDI note for F4
MyNoteButton orangeLow {orangeLowButton, strumUpPin, strumDownPin, orangeChord, &octave, &vel}; // 67 is MIDI note for G4

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

uint8_t getVelocityFromAnalogValue(int analogValue) {
  if (analogValue >= 130 && analogValue <= 190) {
    return ppp; 
  } else if (analogValue >= 350 && analogValue <= 385) {
    return p;
  } else if (analogValue >= 550 && analogValue <= 585) {
    return mf;
  } else if (analogValue >= 760 && analogValue <= 805) {
    return f;
  } else if (analogValue >= 970 && analogValue <= 1023) {
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

  // Serial.begin(9600); // Initialize serial communication

  Control_Surface.begin();
}

void loop() {
  Control_Surface.loop();

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
    delay(200); // debounce delay
  }

    if (digitalRead(upSelect) == LOW) {
    Serial.println("Up button pressed");
    delay(200); // debounce delay
  }
  
  if (digitalRead(downSelect) == LOW) {
    Serial.println("Down button pressed");
    delay(200); // debounce delay
  }
  
  if (digitalRead(leftSelect) == LOW) {
    Serial.println("Left button pressed");
    delay(200); // debounce delay
  }
  
  if (digitalRead(rightSelect) == LOW) {
    Serial.println("Right button pressed");
    delay(200); // debounce delay
  }

  if (digitalRead(okButton) == LOW) {
    Serial.println("OK button pressed");
    delay(200); // debounce delay
  }

  if (digitalRead(backButton) == LOW) {
    Serial.println("Back button pressed");
    delay(200); // debounce delay
  }
}