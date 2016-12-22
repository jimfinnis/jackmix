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
#include "ringbuffer.h"

/// this is a control channel, used to manage a list of values.
/// It has an input range, which converts whatever the incoming
/// data is into 0-1, and then to the output range.
/// If convert is not set, no conversion is done.

/// In general, the input range is used to convert the raw data to
/// 0-1, while the output range is used to convert the data to
/// a special range and is often omitted. Input range must be given.
///
/// The mapping process goes
/// inputrange --inmin/inmax-> 0-1   --outmin/outmax-> output range


class Ctrl {
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
    
    /// if false, no conversion is done
    bool convert;
    
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
    
    void pollRing(){
        float f;
        bool got=false;
        while(ring->read(f)){got=true;}
        if(got){
            std::vector<Value *>::iterator it;
            for(it=values.begin();it!=values.end();it++){
                (*it)->setTarget(f);
            }
        }
    }

public:
    static Ctrl *createOrFind(std::string name);
    
    
    Ctrl(){
        convert=true;
        hasSource=false;
        inmin=0;inmax=1;
        outmin=0;outmax=1;
        ring = new RingBuffer<float>(20);
    }
    
    ~Ctrl(){
        delete ring;
    }
        
    Ctrl *setinrange(float mn,float mx){
        inmin=mn;inmax=mx;
        return this;
    }
    Ctrl *setoutrange(float mn,float mx){
        outmin=mn;outmax=mx;
        return this;
    }
    Ctrl *noconvert(){
        convert=false;
        return this;
    }
    
    /// sets the source for a control - parses the spec crudely
    /// to work out what type of source, and calls code to add the
    /// ctrl to the source type's structures.
    Ctrl *setsource(std::string spec);
    
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
        if(convert){
            v = (v-inmin)/(inmax-inmin);
            v = v*(outmax-outmin)+outmin;
        }
        ring->write(v);
    }
    
    /// Check all values
    /// all have an external control channel.
    static void checkAllCtrlsForSource();
    
    /// call from the jack process thread regularly; will poll
    /// all the ctrl ring buffers, reading data. If there is
    /// any, it will be passed down to the values.
    static void pollAllCtrlRings();
        
};   
    

    

#endif /* __CTRL_H */
