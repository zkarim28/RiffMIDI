// Custom chords built from flash_gui.py's fretboard chord builder. Each
// array lists the actual MIDI note numbers for the strings that are
// played (low to high; muted strings are simply left out). These get
// appended onto the built-in chords[]/chordSizes[]/chordNames[] arrays
// in RiffMIDI.ino. Edit by hand only if you're not using the GUI.
#ifndef CUSTOM_CHORDS_H
#define CUSTOM_CHORDS_H

int customChord0[] = {43, 45, 50, 55, 59, 64};  // gad
int customChord1[] = {60, 45, 50, 55, 59, 64};  // high c
int customChord2[] = {50, 57, 62};  // BV1
int customChord3[] = {45, 52, 57};  // BV2
int customChord4[] = {42, 49, 54};  // BV3
int customChord5[] = {57};  // BVS1
int customChord6[] = {59};  // BVS2

#define CUSTOM_CHORDS_LIST customChord0, customChord1, customChord2, customChord3, customChord4, customChord5, customChord6
#define CUSTOM_CHORD_SIZES_LIST (sizeof(customChord0) / sizeof(int)), (sizeof(customChord1) / sizeof(int)), (sizeof(customChord2) / sizeof(int)), (sizeof(customChord3) / sizeof(int)), (sizeof(customChord4) / sizeof(int)), (sizeof(customChord5) / sizeof(int)), (sizeof(customChord6) / sizeof(int))
#define CUSTOM_CHORD_NAMES_LIST "gad", "high c", "BV1", "BV2", "BV3", "BVS1", "BVS2"

#endif
