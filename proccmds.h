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
          NudgeValue,           // vp,v(amount)
          ChannelMute,          // chan
          ChannelSolo,          // chan
          DelChan,              // chan
          DelSend,              // chan,arg0(send index)
          TogglePrePost,        // chan,arg0(send index)
          AddSend,              // chan,s(name)
          AddEffect,            // pld,s(inst name),arg0(chain idx)
          SetValue,             // vp,v(new val)
          // complex one this:
          //  arg0 is the chain index
          //  s is the effect instance to change
          //  s2 is the input within that instance to remap
          //  arg1 is 0/1 or -1. If -1, we're getting the input from another effect
          //    and the below are used.
          //  s3 is the effect instance to get the output from
          //  s4 is the output within that instance
          RemapInput,           // s(inst name),s2(inp name),s3(inst name for output),s4(out name),arg0(0/1/-1)
          
          // arg0 is chain index,
          // arg1 is the chain output to remap
          // s is instance to use output of
          // s2 is the instance's output which will be the chain's output
          RemapOutput,
          AddChannel            // s(name),arg0(1/2 [mono/stereo])
};

// this is a struct, not a union, because a lot of things can appear here together.
// Yes, I could tidy it up.

struct ProcessCommand {
    ProcessCommand(){}
    static const int STRSIZE=128;
    
    // various random constructors as the commands need them. Ugly.
    ProcessCommand(ProcessCommandType c){
        cmd = c;
    }
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
        cmd = c;        chan = ch;
        setstr(str);
    }
    
    ProcessCommand(ProcessCommandType c,std::string str,int i){
        if(str.size()>STRSIZE) throw _("string too large");
        strcpy(s,str.c_str());
        cmd = c;	arg0 = i;
        setstr(str);
    }
    
    ProcessCommand *setfloat(float f){
        v = f;
        return this;
    }
    
    ProcessCommand *setvalptr(Value *v){
        vp = v;
        return this;
    }
    
    ProcessCommand *setarg0(int i){
        arg0=i;
        return this;
    }
    ProcessCommand *setarg1(int i){
        arg1=i;
        return this;
    }
    
    
    ProcessCommand *setstr(std::string str){
        if(str.size()>STRSIZE) throw _("string too large");
        strcpy(s,str.c_str());
        return this;
    }        
    ProcessCommand *setstr2(std::string str){
        if(str.size()>STRSIZE) throw _("string too large");
        strcpy(s2,str.c_str());
        return this;
    }        
    ProcessCommand *setstr3(std::string str){
        if(str.size()>STRSIZE) throw _("string too large");
        strcpy(s3,str.c_str());
        return this;
    }        
    ProcessCommand *setstr4(std::string str){
        if(str.size()>STRSIZE) throw _("string too large");
        strcpy(s4,str.c_str());
        return this;
    }        
    
    
    ProcessCommandType cmd;
    
    Channel *chan;
    float v;
    Value *vp;
    int arg0; // heaven knows why I've got this name...
    int arg1;
    // can't use std::string (or any STL type) because this
    // gets memcpy's inside jack's ringbuffer.
    char s[STRSIZE];
    char s2[STRSIZE];
    char s3[STRSIZE];
    char s4[STRSIZE];
    
    struct PluginData *pld;
};


#endif /* __PROCCMDS_H */
