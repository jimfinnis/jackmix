/**
 * @file channel.h
 * @brief  Brief description of file.
 *
 */

#ifndef __CHANNEL_H
#define __CHANNEL_H

#include <jack/jack.h>
#include <vector>
#include <string>
#include "value.h"

// largest chunk we can work on (size of temp buffers)
#define BUFSIZE 1024

// input channel structure


class Channel {
    std::string name;
    bool mono;
    
    Value *pan,*gain;
    
    static std::vector<Channel *> chans;
    
    static jack_port_t *makePort(std::string pname){
        extern jack_client_t *client;
        return jack_port_register(
                                  client,
                                  pname.c_str(),
                                  JACK_DEFAULT_AUDIO_TYPE, 
                                  JackPortIsInput, 0);
    }
    
    // mix this channel into the output
    void mix(float *leftout,float *rightout,int offset,int nframes);
    // store the jack buffer pointers, but do not store them between
    // process() calls
    void cachebufs(int nframes){
        left = (float *)jack_port_get_buffer(leftport,nframes);
        right = (float *)jack_port_get_buffer(rightport,nframes);
    }
        
    // for peak metering
    static volatile float peakl,peakr;
    
    
    // if mono, only leftport is used
    jack_port_t *leftport,*rightport;
    
    // these cache the port buffers, but only inside one call to process()
    float *left,*right;
    
public:
    Channel(std::string n,int ch,Value *g,Value *p){
        name = n;
        mono = ch==1;
        gain = g;
        pan = p;
        
        if(mono){
            leftport = makePort(n);
        } else {
            leftport = makePort(n+"_L");
            rightport = makePort(n+"_R");
        }
        chans.push_back(this);
    }
    
    
    // mixes all channels into the output buffer
    static void mixChannels(float *leftout,float *rightout,int offset,int nframes);
    
    // call once per process() to cache port buffer pointers
    static void cacheAllChannelBuffers(int nframes){
        std::vector<Channel *>::iterator it;
        for(it=chans.begin();it!=chans.end();it++){
            (*it)->cachebufs(nframes);
        }
    }
    static void resetPeak(){
        peakl=peakr=0;
    }
    
    static float getPeakL(){
        return peakl;
    }
    static float getPeakR(){
        return peakr;
    }
    
        
};
    
    


#endif /* __CHANNEL_H */
