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
/// data is into 0-1. This is converted to the value range
/// by setTargetConvert() in Value.

class Ctrl {
    /// the input mapping values, which convert the data coming
    /// in into the 0-1 range. 
    float inmin,inmax;
    
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
                (*it)->setTargetConvert(f);
            }
        }
    }

public:
    static Ctrl *createOrFind(std::string name);
    
    
    Ctrl(){
        hasSource=false;
        inmin=0;inmax=1;
        ring = new RingBuffer<float>(20);
    }
    
    ~Ctrl(){
        delete ring;
    }
        
    Ctrl *setinrange(float mn,float mx){
        inmin=mn;inmax=mx;
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
        v = (v-inmin)/(inmax-inmin);
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
