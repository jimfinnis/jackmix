/**
 * @file midi.cpp
 * @brief  Brief description of file.
 *
 */

#include <vector>
#include <unordered_map>
#include "ctrl.h"
#include "midi.h"

using namespace std;

// this is a map of midi cc to a list of controllers which are
// fed by that cc.

static unordered_map<int,vector<Ctrl *>> sources;

inline int getidx(int chan,int cc){
    return chan*128+cc;
}

void addMidiSource(string source, Ctrl *c){
    int cc = atoi(source.c_str());
    int chan = 1;
    if(cc>=0 && cc<127)
        sources.get(getidx(chan,cc)).push_back(c);
}

void removeMidiReferences(Ctrl *c){
    
    for(int i=0;i<128;i++){
        sources[i].erase(std::remove(sources[i].begin(),
                                     sources[i].end(),c),
                         sources[i].end());
    }
}

void feedMidi(int chan, int cc,float val){
    vector<Ctrl *>::iterator it;
    for(it=sources[cc].begin();it!=sources[cc].end();it++){
        (*it)->setval(val);
    }
}
