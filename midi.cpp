/**
 * @file midi.cpp
 * @brief  Brief description of file.
 *
 */

#include <vector>
#include <unordered_map>
#include "ctrl.h"
#include "midi.h"
#include "stringsplit.h"

using namespace std;

MidiSource midi;

// this is a map of midi cc to a list of controllers which are
// fed by that cc.

static unordered_map<int,vector<Ctrl *>> sources;

inline int getidx(int chan,int cc){
    return chan*128+cc;
}

const char* MidiSource::add(string source, Ctrl *c){
    vector<string> v = split(source,':');
    if(v.size()!=2){
        return "Failed : vector size wrong";
    }
    
    int chan = atoi(v[0].c_str());
    int cc = atoi(v[1].c_str());
    if(cc>=0 && cc<127){
        sources[getidx(chan,cc)].push_back(c);
        c->sourceInfo = new MidiSourceInfo(chan,cc);
        c->source = this;
        return NULL;
    }
    return "Failed : CC out of range";
}

void MidiSource::remove(Ctrl *c){
    MidiSourceInfo *info = (MidiSourceInfo*)(c->sourceInfo);
    vector<Ctrl *>& v = sources[getidx(info->chan,info->cc)];
    v.erase(std::remove(v.begin(),v.end(),c),v.end());
    delete info;
}

void MidiSource::feed(int chan, int cc,float val){
    vector<Ctrl *>::iterator it;
    int idx = getidx(chan,cc);
    for(it=sources[idx].begin();it!=sources[idx].end();it++){
        (*it)->setval(val);
    }
}
