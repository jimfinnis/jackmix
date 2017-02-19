/**
 * @file proccmds.h
 * @brief  Brief description of file.
 *
 */

#ifndef __PROCCMDS_H
#define __PROCCMDS_H


// commands sent from main thread code (i.e. InputManager and screens
// to processing thread, with the ProcessCommand elements they require
// in the comment

enum ProcessCommandType {
    ChangeGain,                 // chan,v
          ChangePan,            // chan,v
          ChangeMasterGain,     // v
          ChangeMasterPan,      // v
          ChangeSendGain,       // chan,arg0(send index),v
          ChannelMute,          // chan
          ChannelSolo,          // chan
          DelChan,              // chan
          ChangeEffectParam,    // vp, v
          DelSend,              // chan,arg0(send index)
          TogglePrePost,        // chan,arg0(send index)
          AddSend,              // chan,s(name)
          AddChannel            // s(name),arg0(1/2 [mono/stereo])
};

// this is a struct, not a union, because a lot of things can appear here together.
// Yes, I could tidy it up.

struct ProcessCommand {
    ProcessCommand(){}
    static const int STRSIZE=128;
    
    // various random constructors as the commands need them. Ugly.
    ProcessCommand(ProcessCommandType c,float f, Channel *ch,int a0){
        cmd = c;        v = f;        chan = ch;        arg0 = a0;
    }
    
    ProcessCommand(ProcessCommandType c,float f,Value *p){
        cmd = c;        vp = p;        v = f;
    }
    ProcessCommand(ProcessCommandType c,Channel *ch,float f){
        cmd = c;        chan = ch;     v = f;
    }
    ProcessCommand(ProcessCommandType c,Channel *ch){
        cmd = c;        chan = ch;
    }
    
    
    ProcessCommand(ProcessCommandType c,Channel *ch,std::string str){
        if(str.size()>STRSIZE) throw _("string too large");
        strcpy(s,str.c_str());
        cmd = c;        chan = ch;
    }
    
    ProcessCommand(ProcessCommandType c,std::string str,int i){
        if(str.size()>STRSIZE) throw _("string too large");
        strcpy(s,str.c_str());
        cmd = c;	arg0 = i;
    }
    
    ProcessCommandType cmd;
    Channel *chan;
    float v;
    Value *vp;
    int arg0; // heaven knows why I've got this name...
    // can't use std::string (or any STL type) because this
    // gets memcpy's inside jack's ringbuffer.
    char s[STRSIZE];
};


#endif /* __PROCCMDS_H */
