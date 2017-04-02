/**
 * @file midi.h
 * @brief  Brief description of file.
 *
 */

#ifndef __MIDI_H
#define __MIDI_H

#include "ctrlsource.h"

class MidiSource : public CtrlSource {
public:
    virtual const char * add(std::string source,Ctrl *c);
    virtual void remove(Ctrl *c);
    virtual void setrangedefault(Ctrl *c){
        c->setinrange(0,127);
    }
    virtual const char *getName(){ return "midi"; }
    
    void feed(int chan, int cc,float val);
};

struct MidiSourceInfo : public CtrlSourceInfo {
    int chan,cc;
    MidiSourceInfo(int ch,int c){
        chan = ch;
        cc = c;
    }
};

extern MidiSource midi;

#endif /* __MIDI_H */
