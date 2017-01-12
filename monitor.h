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

#include "exception.h"
#include "channel.h"
#include "timeutils.h"
#include "lineedit.h"
#include "stringlist.h"

using namespace std;

struct ChanMonData {
    void init(string n,float _l,float _r,float g,float p,Channel *c){
        l=_l; r=_r;
        gain=g; pan=p;
        strcpy(name,n.c_str());
        chan=c;
    }
    char name[256];
    // momentary values
    float l,r;
    // sources (don't read the actual value!)
    // these are actual values, but could be smoothed
    // so we can't use the get() data in the monitor thread
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
          ChangeEffectParam,DelSend,TogglePrePost,AddSend,
          AddChannel
};

// this is a struct, not a union, because a lot of things can appear here together.
// Yes, I could tidy it up.

struct MonitorCommand {
    MonitorCommand(){}
    static const int STRSIZE=128;
    
    // various random constructors as the commands need them. Ugly.
    MonitorCommand(MonitorCommandType c,float f, Channel *ch,int a0){
        cmd = c;        v = f;        chan = ch;        arg0 = a0;
    }
    
    MonitorCommand(MonitorCommandType c,float f,Value *p){
        cmd = c;        vp = p;        v = f;
    }
    
    MonitorCommand(MonitorCommandType c,Channel *ch,std::string str){
        if(str.size()>STRSIZE) throw _("string too large");
        strcpy(s,str.c_str());
        cmd = c;        chan = ch;
    }
    
    MonitorCommand(MonitorCommandType c,std::string str,int i){
        if(str.size()>STRSIZE) throw _("string too large");
        strcpy(s,str.c_str());
        cmd = c;	arg0 = i;
    }
    
    MonitorCommandType cmd;
    Channel *chan;
    float v;
    Value *vp;
    int arg0; // heaven knows why I've got this name...
    // can't use std::string (or any STL type) because this
    // gets memcpy's inside jack's ringbuffer.
    char s[STRSIZE];
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
    // current send - -ve if not valid
    int cursend=-1;
    // current channel or NULL
    Channel *curchanptr=NULL;
    
    // keymethods return true if key accepted
    typedef bool(MonitorUI::*KeyMethod)(int c);
    // method to call if we get a key or NULL if
    // we're not waiting for one
    KeyMethod keyMethod=NULL;
    std::string keyString; // string to show if asking for key
    
    void getKey(std::string s,KeyMethod m){ // prompt, method to call
        keyString = s;
        keyMethod = m;
    }
    
    // status line
    string statusMsg;
    bool statusShowing=false;
    Time statusTimeToEnd;
    
    typedef void (MonitorUI::*StringMethod)(std::string);
    
    // string list selection
    StringList stringList;
    StringMethod stringFinishedMethod=NULL;
    void beginStringList(std::string p,
                         std::vector<std::string>& l, StringMethod m){
        stringList.begin(p,l);
        stringFinishedMethod = m;
    }
    
    
    // line editing
    LineEdit lineEdit;
    StringMethod lineFinishedMethod=NULL;
    void beginLineEdit(std::string s, StringMethod m){
        lineEdit.begin(s);
        lineFinishedMethod = m;
    }
    
    // callback methods for editors
    void lineFinishedSaveFile(std::string s);
    void lineFinishedAddChannelMono(std::string s);
    void lineFinishedAddChannelStereo(std::string s);
    void stringFinishedAddChain(std::string s);
    bool confirmDeleteSend(int key);
    bool keyMonoOrStereoChannel(int key);
    
    
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
