/**
 * @file ctrl.h
 * @brief  Brief description of file.
 *
 */

#ifndef __CTRL_H
#define __CTRL_H

#include <string>
#include <unordered_map>
#include <vector>
#include "value.h"

/// this is a control channel, used to manage a list of values.
/// It has an input range, which converts whatever the incoming
/// data is into 0-1, or -60-0 for log ctrls.

/// In general, the input range is used to convert the raw data to
/// 0-1, while the output range is used to convert the data to
/// a special range and is often omitted. Input range must be given.
///
/// The mapping process goes
/// inputrange --inmin/inmax-> 0-1   --outmin/outmax-> output range
/// or
/// inputrange --inmin/inmax-> -60-0 --outmin/outmax-> output range
///


class Ctrl {
    /// is this a -60 to 0 decibel scale, or a 0 to 1 linear scale?
    bool logscale;
    /// the input mapping values, which convert the data coming
    /// in into the 0-1 range. If logscale is true,
    /// they instead map onto the -60 - 0 range.
    float inmin,inmax;
    
    /// the output mapping values, which convert the data in 0-1
    /// range (i.e. after input mapping and any decibel->ratio
    /// conversion) into the final value. You'd probably not use
    /// these for logscale, but it is still available.
    float outmin,outmax;
    
    /// A list of the values this ctrl controls
    std::vector<Value *> values;
    
    /// and there is a map of all the control channels
    static std::unordered_map<std::string,Ctrl *> map;
    
public:
    static Ctrl *createOrFind(std::string name);
    
    
    Ctrl(){
        logscale=false;
        inmin=0;inmax=1;
        outmin=0;outmax=1;
    }
        
    // fluent modifiers
    Ctrl *setlog(){
        logscale = true;
        return this;
    }
    Ctrl *setinrange(float mn,float mx){
        inmin=mn;inmax=mx;
        return this;
    }
    Ctrl *setoutrange(float mn,float mx){
        outmin=mn;outmax=mx;
        return this;
    }
    
    /// add a value to be managed by this control
    void addval(Value *v){
        values.push_back(v);
    }
    
    /// set a value; will convert the input then pass it down to the
    /// individual values
    void setval(float v){
        v = (v-inmin)/(inmax-inmin);
        if(logscale)v=v*60.0f-60.0f;
        v = v*(outmax-outmin)+outmin;
        
        std::vector<Value *>::iterator it;
        for(it=values.begin();it!=values.end();it++){
            (*it)->value = v;
        }
    }
};   
    

    

#endif /* __CTRL_H */
