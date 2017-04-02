/**
 * @file midi.h
 * @brief  Brief description of file.
 *
 */

#ifndef __MIDI_H
#define __MIDI_H

void addMidiSource(std::string source, Ctrl *c);
void removeMidiReferences(Ctrl *c);
void feedMidi(int chan,int cc,float val);


#endif /* __MIDI_H */
