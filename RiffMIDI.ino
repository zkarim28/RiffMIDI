#include <Control_Surface.h>

USBMIDI_Interface midi;

// Plus and Minus Buttons
// CCIncrementDecrementButtons

// //Pitch Bend Potentiometer (not the full range, need to use range func)
// PBPotentiometer whammyBar {
//   A0,
//   Channel_1,
// };

// // Possibly control the scale of the 
// CCPotentiometer fiveWayToggle {
//   A1,
//   Channel_1,
// };

class MyNoteButton : public MIDIOutputElement {
 public:
  /*
   * @brief   Create a new MyNoteButton object on the given pin, with the 
   *          given address and velocity.
   * 
   * @param   pin
   *          The digital input pin to read from.  
   *          The internal pull-up resistor will be enabled.
   * @param   address
   *          The MIDI address to send to.
   * @param   velocity
   *          The MIDI note velocity [0, 127].
   */
  MyNoteButton(pin_t notePin, pin_t strumUpPin, pin_t strumDownPin, MIDIAddress address, uint8_t velocity)
    : noteButton(notePin), strumUpSwitch(strumUpPin), strumDownSwitch(strumDownPin), address(address), velocity(velocity) {}
 
 public:
  // Initialize: enable the pull-up resistor for the button
  // This method is called once by `Control_Surface.begin()`.
  void begin() final override { 
    noteButton.begin(); 
    strumUpSwitch.begin();
    strumDownSwitch.begin();
  }
 
  // Update: read the button and send MIDI messages when appropriate.
  // This method is called continuously by `Control_Surface.loop()`.
  void update() final override {
    AH::Button::State noteState = noteButton.update();        // Read the button
    AH::Button::State strumUpState = strumUpSwitch.update();
    AH::Button::State strumDownState = strumDownSwitch.update();
    if (noteState == AH::Button::Pressed && strumUpState == AH::Button::Falling) {               // if pressed
      Control_Surface.sendNoteOn(address, velocity);  //   → note on
    } else if (noteState == AH::Button::Rising) {         // if released
      Control_Surface.sendNoteOff(address, velocity); //   → note off
    }
  }
 
 private:
  AH::Button noteButton;
  AH::Button strumUpSwitch;
  AH::Button strumDownSwitch;
  const MIDIAddress address;
  uint8_t velocity;
};

int strumDownPin = 2;
int strumUpPin = 3;

int octaveUpPin = 7;
int octaveDownPin = 8;

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

int lowButtonOctave = 4;
int highButtonOctave = 5;

int fiveSelectSwitch = A1;
int fiveSelectSwitchVal = 0;

int vel = 127;

// NoteButton test1 {
//   strumUpPin,
//   {MIDI_Notes::C[5], Channel_1}
// };

// NoteButton test2 {
//   strumDownPin,
//   {MIDI_Notes::C[5], Channel_1}
// };

//STRUM DOWN BROKEN, MUST FIX

// Bottom Buttons simply work as a tap per se
NoteButton greenHigh {
  greenHighButton,
  {MIDI_Notes::C[5], Channel_1}
};

NoteButton redHigh {
  redHighButton,
  {MIDI_Notes::D[5], Channel_1}
};

NoteButton yellowHigh {
  yellowHighButton,
  {MIDI_Notes::E[5], Channel_1}
};

NoteButton blueHigh {
  blueHighButton,
  {MIDI_Notes::G[5], Channel_1}
};

NoteButton orangeHigh {
  orangeHighButton,
  {MIDI_Notes::A[5], Channel_1}
};

// Top Buttons for chords
MyNoteButton greenLow {
  greenLowButton,
  strumUpPin,
  strumDownPin,
  {MIDI_Notes::C[4], Channel_1},
  vel
};

MyNoteButton redLow {
  redLowButton,
  strumUpPin,
  strumDownPin,
  {MIDI_Notes::D[4], Channel_1},
  vel
};

MyNoteButton yellowLow {
  yellowLowButton,
  strumUpPin,
  strumDownPin,
  {MIDI_Notes::Eb[4], Channel_1},
  vel
};

MyNoteButton blueLow {
  blueLowButton,
  strumUpPin,
  strumDownPin,
  {MIDI_Notes::F[4], Channel_1},
  vel
};

MyNoteButton orangeLow {
  orangeLowButton,
  strumUpPin,
  strumDownPin,
  {MIDI_Notes::G[4], Channel_1},
  vel
};

//there will be a control param from the
// fiveSelectSwitch to change velocity

//pp = 32
//p = 48
//mp = 64
//mf = 80
//f = 96

//possible idea 
//Lowkey I could just use the overdrive to go full ff
//fff = 127

// void strumUp(int[] noteArray, int vel) {
//   if (chordOn == 0 && digitalRead(greenTopButton) == LOW) {
//     for (auto note : greenChordList) {
//       midi.sendNoteOn(midiAddress, vel)
//     }
//   }
// }

// void strumDown(int[] noteArray, int vel) {
//   if (chordOn == 0 && digitalRead(greenTopButton) == LOW) {
//     for (auto note : greenChordList) {
//       noteID = MIDI_Notes::C[4];
//       midi.sendNoteOn(midiAddress, vel)
//     }
//   }
// }

void setup() {
  // put your setup code here, to run once:
  Control_Surface.begin();
  // Serial.begin(9600); 
}

void loop() {
  // put your main code here, to run repeatedly:
  Control_Surface.loop();

  // fiveSelectSwitchVal = analogRead(A1);
  // Serial.println(fiveSelectSwitchVal);
  // delay(100);
}

//If the fiveSelectSwitch val falls in any of these ranges:
//Select one = 130 - 190 -> vel = 48 (p)
//Select two = 350 -  385 -> vel = 64 (mp)
//Select three = 550 - 585 -> vel = 80 (mf)
//Select four = 760 - 805 -> vel = 96 (f)
//Select Five = 970 - 1023 -> vel = 112 (ff)
