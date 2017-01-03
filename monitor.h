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
#include "timeutils.h"

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
          ChangeMasterPan,ChangeSendGain,ChannelMute,ChannelSolo,
          ChangeEffectParam
};

struct MonitorCommand {
    MonitorCommand(){}
    
    MonitorCommand(MonitorCommandType c,float f, Channel *ch,int a0){
        cmd = c;
        v = f;
        chan = ch;
        arg0 = a0;
    }
    
    MonitorCommand(MonitorCommandType c,float f,Value *p){
        cmd = c;
        vp = p;
        v = f;
    }
    
    MonitorCommandType cmd;
    Channel *chan;
    float v;
    Value *vp;
    int arg0;
};


struct StatusLine {
    void setmsg(string s);
    void display();
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
    // current chan in main view, -1 is master, anything else is index
    // into MonitorData.
    int curchan=-1;
    // cleared by state changes, reflects currently edited param in
    // some states.
    int curparam=0; 
    
    // status line
    string statusMsg;
    bool statusShowing=false;
    Time statusTimeToEnd;
    
    void setStatus(string s,double t); // msg, time to show
    void displayStatus();
    
    enum UIState {
        Main,ChanZoom,ChainList
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
        curparam = 0;
    }
    void gotoPrevState(){
        state = prevState;
    }
        
    
    enum BarMode { Gain,Green,Pan };
    
    // states
    void displayMain(MonitorData *d);
    void displayChan(int i,ChanMonData* c,bool cur); // c=NULL if invalid
    
    void displayChanZoom(MonitorData *c);
    
    void displayChainList();
    
    // v is 0-1 linear unless rv (range value) is present. We treat v and rv separately
    // so that we can store the value to show, passing it from process to main thread in
    // a ring buffer.
    void drawVertBar(int y, int x, int h, int w, 
                     float v,Value *rv,BarMode mode,bool bold);
    void drawHorzBar(int y, int x, int h, int w, 
                     float v,Value *rv,BarMode mode,bool bold);
    
    // display page title
    void title(const char *s);
    
    // nudge the gain of the currently edited send on the current channel
    // in ChanZoom state.
    void commandSendGainNudge(float v);
    // nudge the gain of either the current channel or the master
    // if curchan<0
    void commandGainNudge(float v);
    // nudge the pan of either the current channel or the master
    // if curchan<0
    void commandPanNudge(float v);
    // nudge the current effect parameter
    void commandParamNudge(float v);
    
    // generic command to current chan, if valid
    void simpleChannelCommand(MonitorCommandType cmd);
    
    // send some command
    void command(MonitorCommandType cmd,float v,Channel *c=NULL,int arg0=0);
    
};



#endif /* __MONITOR_H */
