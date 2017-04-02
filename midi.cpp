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

// if we are waiting for some cc input to determine which midi cc
// is to control a ctrl, then this is the ctrl that's waiting.

static Ctrl *waitingForInput = NULL;
static int recordedInput = -1;
inline int getidx(int chan,int cc){
    return chan*128+cc;
}

const char* MidiSource::add(string source, Ctrl *c){
    if(source == "input"){
        if(waitingForInput) return "Already waiting.";
        c->sourceInfo = new MidiSourceInfo(-1,-1);
        waitingForInput = c;
        c->source = this;
        return NULL;
    } else {
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
}

void MidiSource::remove(Ctrl *c){
    MidiSourceInfo *info = (MidiSourceInfo*)(c->sourceInfo);
    vector<Ctrl *>& v = sources[getidx(info->chan,info->cc)];
    v.erase(std::remove(v.begin(),v.end(),c),v.end());
    delete info;
}

void MidiSource::feed(int chan, int cc,float val){
    if(waitingForInput){
        if(recordedInput<0)
            recordedInput = val;
        
        if(abs(recordedInput-val)>20){
            MidiSourceInfo *info = (MidiSourceInfo*)(waitingForInput->sourceInfo);
            info->chan = chan;
            info->cc = cc;
            stringstream ss;
            ss << chan << ":" << cc;
            waitingForInput->sourceString = ss.str();
            sources[getidx(chan,cc)].push_back(waitingForInput);
            waitingForInput = NULL;
            recordedInput = -1;
        }
    }
    
    vector<Ctrl *>::iterator it;
    int idx = getidx(chan,cc);
    for(it=sources[idx].begin();it!=sources[idx].end();it++){
        (*it)->setval(val);
    }
}
