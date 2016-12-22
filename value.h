
/**
 * @file value.h
 * @brief  Brief description of file.
 *
 */

#ifndef __VALUE_H
#define __VALUE_H

#include <math.h>
#include <vector>

// these are the values used in the mixer: pan positions, gains etc.
// and effect parameters. They can either be constants, or have a default
// value but be modifiable from a control channel.

class Value {
    friend class Ctrl;
    
    /// the post-conversion default value
    float deflt;
    /// the current value, set from the LPF
    float value;
    /// the target value, input to the LPF
    float target;
    /// the smoothing value for updates - the closer to 1, the more slowly
    /// the value tracks the target. 0.5 (the default) is really quick,
    /// 0.9 gives a nice smoothness. 0.99 is very slow, useful for crossfades.
    /// value(t) = value(t-1)*smooth + target*(1-smooth)
    float smooth;
    
    /// if true, value is expected to be -60 - 0 and is converted
    /// to a ratio 0-1 on get. Any ctrl value should also be log
    /// scale.
    bool db;
    
    /// list of all values
    static std::vector<Value *> values;
    
    
public:
    Value(){
        deflt=0;
        smooth=0.5;
        db=false;
        values.push_back(this);
    }
    
    /// set the default
    Value *setdef(float def){
        deflt=def;
        return this;
    }
    /// set DB (decibel conversion will be done on get)
    Value *setdb(){
        db=true;
        return this;
    }
    /// set smoothing factor (see notes for smooth, above)
    Value *setsmooth(float s){
        smooth=s;
        return this;
    }
    
    /// set the value from a control - actually sets the target
    /// value of an LPF to avoid artifacts. The value itself is set
    /// in update(). Performs db conversion.
    
    void setTarget(float v){
        if(db)
            target=powf(10.0,v*0.1f);
        else
            target=v;
    }
    
    /// get the value
    float get(){
        return value;
    }
    
    /// get a pointer to the value - ONLY use this when you
    /// connect LADSPA control ports
    float *getAddr(){
        return &value;
    }
            
    
    /// reset value and target to default
    void reset(){
        value=deflt;
        target=deflt;
    }
    
    /// perform periodic update
    void update(){
        value = value*smooth + target*(1.0f-smooth);
    }
    
    /// update all values
    static void updateAll();
        
    
          
};   
                
        
        
    
    


#endif /* __VALUE_H */
