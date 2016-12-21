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
#include "ring.h"

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
    bool db;
    /// the input mapping values, which convert the data coming
    /// in into the 0-1 range. If db is true,
    /// they instead map onto the -60 - 0 range.
    float inmin,inmax;
    
    /// the output mapping values, which convert the data in 0-1
    /// or -60 to 0 range
    /// range (i.e. after input mapping into the final value.
    /// You'd probably not use these for db, but it is still
    /// available. Decibel->ratio conversion is done in the value.
    float outmin,outmax;
    
    /// A list of the values this ctrl controls
    std::vector<Value *> values;
    
    /// and there is a map of all the control channels
    static std::unordered_map<std::string,Ctrl *> map;
    
    /// true if this ctrl has an external data source defined
    bool hasSource;
    
    /// this is a ring buffer - it is used to pass ctrl change data
    /// from the reader thread for particular source types into
    /// the process thread, where values can be changed.
    RingBuffer<float> *ring;
    

    /// this code is called inside the Jack process thread:
    /// it checks the ring buffer for any new setting, and passes
    /// it onto the values.
    
    void ringPoll(){
        float f;
        if(ring->read(f)){
            std::vector<Value *>::iterator it;
            for(it=values.begin();it!=values.end();it++){
                (*it)->setTarget(f);
            }
        }
    }
    

public:
    static Ctrl *createOrFind(std::string name);
    
    
    Ctrl(){
        db=false;
        hasSource=false;
        inmin=0;inmax=1;
        outmin=0;outmax=1;
        ring = new RingBuffer<float>(20);
    }
    
    ~Ctrl(){
        delete ring;
    }
        
    // fluent modifiers
    Ctrl *setdb(){
        db = true;
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
    Ctrl *setsource(std::string spec){
        hasSource=true;
        printf("setsource tbd!!!!!!!!!!!\n");
        return this;
    }
    
    /// add a value to be managed by this control. DB controllers
    /// must control DB values, but we don't check that here - we
    /// do it after all parsing.
    void addval(Value *v){
        values.push_back(v);
    }
    
    /// set a value; will convert the input then pass it down to the
    /// individual values, VIA THE RING BUFFER. It should be called
    /// from the source handler thread, and from one thread only
    /// for each control. It will just fill the ring and then
    /// do nothing if ringPoll() isn't regularly called.
    void setval(float v){
        v = (v-inmin)/(inmax-inmin);
        if(db)v=v*60.0f-60.0f;
        v = v*(outmax-outmin)+outmin;
        ring->write(v);
    }
    
    /// make sure all the values in all db controls are db, and
    /// all values in non-db controls are non-db. Also check they
    /// all have an external control channel.
    static void checkAllCtrlsForValueDBAgreementAndSource();
        
};   
    

    

#endif /* __CTRL_H */
