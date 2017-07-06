
/**
 * @file value.h
 * @brief  Brief description of file.
 *
 */

#ifndef __VALUE_H
#define __VALUE_H

#include <math.h>
#include <vector>
#include <string>
#include <iostream>

// optsset flags
#define VALOPTS_MIN 1
#define VALOPTS_MAX 2

// standard range for dB gains
#define MINDB -60.0f
#define MAXDB 6.0f

// these are the values used in the mixer: pan positions, gains etc.
// and effect parameters. They can either be constants, or have a default
// value but be modifiable from a control channel.

class Value {
    friend class Ctrl;
    
    /// the current value, set from the LPF
    float value;
    /// the post-conversion default value
    float deflt;
    /// the target value, input to the LPF
    float target;
    /// the smoothing value for updates - the closer to 1, the more slowly
    /// the value tracks the target. 0.5 (the default) is really quick,
    /// 0.9 gives a nice smoothness. 0.99 is very slow, useful for crossfades.
    /// value(t) = value(t-1)*smooth + target*(1-smooth)
    float smooth;
    
    /// list of all values
    static std::vector<Value *> values;
    
    /// what external controller, if any, is controlling me. Generally
    /// used only for information (saving, monitoring).
    class Ctrl *ctrl;
    
public:
    Value(std::string nm){
        name = nm;
        deflt=0;
        smooth=0.5;
        db=false;
        mn=0;mx=1;
        ctrl=NULL;
        values.push_back(this);
        optsset=0;
    }
    
    class Ctrl *getCtrl(){
        return ctrl;
    }
    
    // remove this controller from all values
    static void removeCtrl(Ctrl *c);    
    // list names for debug
    static void dump();
        
    
    ~Value();
    
    float *getptr(){
        return &value;
    }
    
    /// an identifying name used in the controller screen, and not
    /// many other places
    std::string name;
    
    /// which options (max, min etc.) have been used if it's hard to
    /// tell by just examining the value - used for saving config.
    int optsset;
    /// Range -  typically -60 to 0 for DB but doesn't have to be.
    float mn,mx;
    /// if true, value is converted on get()
    bool db;
    
    /// set the default
    Value *setdef(float def){
        deflt=def;
        return this;
    }
    Value *setctrl(Ctrl *c){
        ctrl = c;
        return this;
    }
              
    /// set DB (decibel conversion will be done in get()))
    Value *setdb(){
        db=true;
        return this;
    }
    /// set smoothing factor (see notes for smooth, above)
    Value *setsmooth(float s){
        smooth=s;
        return this;
    }
    /// set range
    Value *setrange(float a,float b){
        mn=a;mx=b;
        return this;
    }
    
    /// set the standard range for dB gains
    Value *setdbrange(){
        return setrange(MINDB,MAXDB);
    }
    
    /// set the name
    Value *setname(std::string nm){
        name = nm;
        return this;
    }
    
    /// set the value - actually sets the target
    /// value of an LPF to avoid artifacts. The value itself is set
    /// in update().
    
    void setTarget(float v){
        if(v>mx)v=mx;
        if(v<mn)v=mn;
        target=v;
    }
    
    /// sets the target value, converting from 0-1 first.
    void setTargetConvert(float f){
        f *= (mx-mn);
        f += mn;
        setTarget(f);
    }
    
    /// convert from target value to 0-1 range (used for nudge,etc.)
    float convertFromTarget(){
        return (target-mn)/(mx-mn);
    }
    
    /// get the value for actual use (i.e. do dB conversion)
    float get(){
        if(db)
            return powf(10.0,value*0.1f);
        else
            return value;
    }
    /// get the value without dB conversion
    float getNoDBConvert(){
        return value;
    }
    
    /// get a pointer to the value - ONLY use this when you
    /// connect LADSPA control ports
    float *getAddr(){
        return &value;
    }
            
    
    /// reset value and target to default
    Value* reset(){
        value = target = deflt;
        return this;
    }
    
    /// perform periodic update
    void update(){
        value = value*smooth + target*(1.0f-smooth);
    }
    
    /// update all values
    static void updateAll();
    
    /// convert to a string for saving
    std::string toString();
    
    
    void nudge(float v){
        float f = convertFromTarget();
        f += v*0.1f;
        if(f<0)f=0;
        if(f>1)f=1;
        setTargetConvert(f);
    }
    
          
};   
                
        
        
    
    


#endif /* __VALUE_H */
