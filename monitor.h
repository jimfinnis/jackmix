/**
 * @file monitor.h
 * @brief Graphical monitoring system. Monitor data is kept
 * in a block with data for all channels. This block is
 * sent from the processing thread to the main thread in
 * a ringbuffer. Data is displayed with curses.
 *
 */

#ifndef __MONITOR_H
#define __MONITOR_H

#include <vector>
#include <string>
#include <string.h>

#include "channel.h"

using namespace std;

struct ChanMonData {
    void init(string n,float _l,float _r,float g,float p,Channel *c){
        l=_l; r=_r;
        gain=g; pan=p;
        strcpy(name,n.c_str());
        chan=c;
    }
    char name[256];
    float l,r;
    float gain,pan;
    Channel *chan;
};

struct MonitorData {
    ChanMonData master = {"MASTER",0,0,1,0.5,NULL};
    // annoyingly the ring buffer doesn't like vectors or strings
    int numchans=0;
    ChanMonData chans[32];
    
    ChanMonData *add(){
        if(numchans<32){
            return chans+(numchans++);
        } else return NULL;
    }
};

// commands sent from main thread monitor code to processing thread

enum MonitorCommandType {ChangeGain,ChangePan,ChangeMasterGain,
          ChangeMasterPan};

struct MonitorCommand {
    MonitorCommand(){}
    
    MonitorCommand(MonitorCommandType c,float f, Channel *ch){
        cmd = c;
        v = f;
        chan = ch;
    }
    MonitorCommandType cmd;
    Channel *chan;
    float v;
};




class MonitorUIDummy {
public:
    MonitorUIDummy(){}
    ~MonitorUIDummy(){}
    
    void display(MonitorData *d){}
    void handleInput(){};
};

class MonitorUIBasic {
public:
    MonitorUIBasic(){}
    ~MonitorUIBasic(){}
    
    void display(MonitorData *d);
    void handleInput(){};
};

class MonitorUI {
    int w,h; // display size
    int curchan=0;
    
    enum UIState {
        Main,Help
          };
    
    UIState state;
    UIState prevState;
public:
    MonitorUI();
    ~MonitorUI();
    
    void display(MonitorData *d);
    void handleInput();

private:
    const char ***helpScreen;
    void gotoState(UIState s){
        prevState = state;
        state = s;
    }
    void gotoPrevState(){
        state = prevState;
    }
        
    
    enum BarMode { Gain,Green,Pan };
    
    void displayChans(MonitorData *d);
    void displayChan(int i,ChanMonData* c,bool cur); // c=NULL if invalid
    void drawVertBar(int y, int x, int h, int w, 
                     float v,BarMode mode,bool bold);


    void commandGainNudge(float v);
    void commandPanNudge(float v);
    void command(MonitorCommandType cmd,float v,Channel *c);
};



#endif /* __MONITOR_H */
