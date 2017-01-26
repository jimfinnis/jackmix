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

extern class Screen *curscreen; // the current screen. LOCK IT.

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

// commands sent from main thread monitor code to processing thread,
// with the MonitorCommand elements they require in the comment

enum MonitorCommandType {
    ChangeGain,                 // chan,v
          ChangePan,            // chan,v
          ChangeMasterGain,     // v
          ChangeMasterPan,      // v
          ChangeSendGain,       // chan,arg0(send index),v
          ChannelMute,          // chan
          ChannelSolo,          // chan
          ChangeEffectParam,    // vp, v
          DelSend,              // chan,arg0(send index)
          TogglePrePost,        // chan,arg0(send index)
          AddSend,              // chan,s(name)
          AddChannel            // s(name),arg0(1/2 [mono/stereo])
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
    MonitorCommand(MonitorCommandType c,Channel *ch,float f){
        cmd = c;        chan = ch;     v = f;
    }
    MonitorCommand(MonitorCommandType c,Channel *ch){
        cmd = c;        chan = ch;
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


enum InputRequestType {
    InputReqIdle,InputReqLineEdit,InputReqKey,InputReqStringList
};
    
// the input request/response - lock the mutex when you edit or read it.
// This is a global structure - some elements (the list) will need to be cleared
// each time.
struct InputRequest {
    // if t is NOT idle and we're not running, initialise as necessary
    // if t is NOT idle and we are running, process an existing request
    InputRequestType t=InputReqIdle;
    bool running=false;
    // the request - I'm not going to risk putting this lot into a union.
    std::string prompt; // prompt string - for getKey() may be empty
    std::string validKeys; // used for getKey(), if empty any key is fine
    std::vector<std::string> list; // list of strings for StringList
    
    // the result
    bool aborted; // true if we should ignore the result
    std::string strout;
    int intout;
    
    // called when the blocking thread sets up the structure
    void startRequest(InputRequestType tp){
        list.clear();
        running=false;
        aborted=false;
        t = tp;
    }
    
    // called once the non-block thread has completed filling the structure
    // and is about to signal the blocking thread
    void setDone(){
        running=false;
        t = InputReqIdle;
    }
};


// this class controls the non-blocking UI thread, which actually
// draws the display over and over again.
// It receives messages from the jack thread telling it what to display,
// and is also controlled (via a bunch of stuff guarded by a mutex)
// by the blocking UI thread. It runs the display() method in Screen subclasses.

class MonitorThread {
    
    // these variables should be locked
    bool requestStop,running;
    
    // status line
    string statusMsg;
    bool statusShowing=false;
    Time statusTimeToEnd;
    
    // editors
    LineEdit lineEdit;
    StringList stringList;
    
    void loop(); // the main loop

    void displayStatus();
    
    static MonitorThread *instance;
public:
    int w,h; // screen dimensions
    
    // there's only one of these - return this to get the instance. Ugly.
    static MonitorThread *get(){
        return instance;
    }
    
    static void lock();
    static void unlock();
    void setStatus(string s,double t); // msg, time to show
    
    MonitorThread();
    ~MonitorThread();
    
    void threadfunc();
    static void showHelp(const char ***h);
};

// this class handles the blocking IO running in the main thread. It sends messages
// to the jack thread, and also controls the monitor (non-blocking IO) thread through
// a mutexed block of data. It runs the flow() method in Screen subclasses forever,
// until one of those returns false indicating it's time to quit.

class InputManager {
public:
    InputManager(){
    }
    
    void flow();
    void lock();
    void unlock();
    
    // calls lock() and unlock() around a call to the monitor thread's code
    void setStatus(string s,double t); // msg, time to show
    
    // blocking input routines - these set a request and then wait for a condition
    // variable. They then read back the result.
    std::string getString(std::string p,bool *aborted);
    int getKey(
               const char *prompt=NULL,  // might be null, in which case there's no prompt
               const char *keys=NULL     // might be null, in which case any key is fine
               ); 
    std::string getFromList(std::string p,
                            std::vector<std::string>& l,
                            bool *aborted); // may return ""
};



#endif /* __MONITOR_H */
