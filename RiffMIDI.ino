#include <Control_Surface.h>

USBMIDI_Interface midi;

class MyNoteButton : public MIDIOutputElement {
 public:
  MyNoteButton(pin_t notePin, pin_t strumUpPin, pin_t strumDownPin, MIDIAddress address, uint8_t* velocityPtr)
    : noteButton(notePin), strumUpSwitch(strumUpPin), strumDownSwitch(strumDownPin), address(address), velocityPtr(velocityPtr) {}

  void begin() final override { 
    noteButton.begin(); 
    strumUpSwitch.begin();
    strumDownSwitch.begin();
  }

  void update() final override {
    AH::Button::State noteState = noteButton.update();
    AH::Button::State strumUpState = strumUpSwitch.update();
    AH::Button::State strumDownState = strumDownSwitch.update();

    if (noteState == AH::Button::Pressed && strumUpState == AH::Button::Falling) {
      Control_Surface.sendNoteOn(address, *velocityPtr);  // Use the current velocity value
    } else if (noteState == AH::Button::Rising) {
      Control_Surface.sendNoteOff(address, *velocityPtr); // Use the current velocity value
    }
  }

 private:
  AH::Button noteButton;
  AH::Button strumUpSwitch;
  AH::Button strumDownSwitch;
  const MIDIAddress address;
  uint8_t* velocityPtr;  // Pointer to velocity value
};

class MyHighNoteButton : public MIDIOutputElement {
 public:
  MyHighNoteButton(pin_t notePin, MIDIAddress address, uint8_t* velocityPtr)
    : noteButton(notePin), address(address), velocityPtr(velocityPtr) {}

  void begin() final override { 
    noteButton.begin(); 
  }

  void update() final override {
    AH::Button::State noteState = noteButton.update();

    if (noteState == AH::Button::Falling) {
      Control_Surface.sendNoteOn(address, *velocityPtr);  // Use the current velocity value
    } else if (noteState == AH::Button::Rising) {
      Control_Surface.sendNoteOff(address, *velocityPtr); // Use the current velocity value
    }
  }

 private:
  AH::Button noteButton;
  const MIDIAddress address;
  uint8_t* velocityPtr;  // Pointer to velocity value
};

// Pin definitions
int strumDownPin = 2;
int strumUpPin = 3;

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

int fiveSelectSwitch = A1;
uint8_t vel = 127;

int ppp = 16;
int pp = 32;
int p = 48;
int mp = 64;
int mf = 80;
int f = 96;
int ff = 112;
int fff = 127;

// Low Buttons for chords
MyNoteButton greenLow {greenLowButton, strumUpPin, strumDownPin, {MIDI_Notes::C[4], Channel_1}, &vel};
MyNoteButton redLow {redLowButton, strumUpPin, strumDownPin, {MIDI_Notes::D[4], Channel_1}, &vel};
MyNoteButton yellowLow {yellowLowButton, strumUpPin, strumDownPin, {MIDI_Notes::Eb[4], Channel_1}, &vel};
MyNoteButton blueLow {blueLowButton, strumUpPin, strumDownPin, {MIDI_Notes::F[4], Channel_1}, &vel};
MyNoteButton orangeLow {orangeLowButton, strumUpPin, strumDownPin, {MIDI_Notes::G[4], Channel_1}, &vel};

// High Buttons for single notes
MyHighNoteButton greenHigh {greenHighButton, {MIDI_Notes::C[4], Channel_1}, &vel};
MyHighNoteButton redHigh {redHighButton, {MIDI_Notes::D[4], Channel_1}, &vel};
MyHighNoteButton yellowHigh {yellowHighButton, {MIDI_Notes::E[4], Channel_1}, &vel};
MyHighNoteButton blueHigh {blueHighButton, {MIDI_Notes::G[4], Channel_1}, &vel};
MyHighNoteButton orangeHigh {orangeHighButton, {MIDI_Notes::A[4], Channel_1}, &vel};

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
  Control_Surface.begin();
}

void loop() {
  Control_Surface.loop();

  // Read the value from the potentiometer
  int fiveSelectSwitchVal = analogRead(fiveSelectSwitch);
  vel = getVelocityFromAnalogValue(fiveSelectSwitchVal);
}
  //if i want to add accent notes for after strumDown or strumUP, make a timer to count for about 1 second to allow for other notes to play wihtout strum
  //Also make the octaves the same for both high and low buttons
  //change highButton to chord button and lowButton to noteButton
  //