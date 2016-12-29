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
#include "global.h"
#include "fx.h"

// info describing how a chain is fed
struct ChainFeed {
    // ctor just sets up gain and postfade, chain is set later
    ChainFeed(Value *v,bool pf){
        gain = v;
        postfade = pf;
    }
    Value *gain;
    ChainInterface *chain;
    bool postfade;
};

// input channel structure


class Channel {
    std::string name;
    // if ports are null, this is the name of an FX chain. 
    // The left and right buffers will be set to that chain.
    std::string returnChainName;
    bool mono;
    
    Value *pan,*gain;
    
    static std::vector<Channel *> inputchans,returnchans;
    
    // after parsing, is traversed to build the actual chain pointers.
    // Is then not used.
    static std::vector<std::string> chainNames;
    
    // pointers to actual chains, built from chainNames after parsing.
    static std::vector<ChainFeed> chains;
    
    
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
    // process() calls. Does not do anything with return channels.
    void cachebufs(int nframes){
        if(leftport)
            left = (float *)jack_port_get_buffer(leftport,nframes);
        if(rightport)
            right = (float *)jack_port_get_buffer(rightport,nframes);
    }
        
    // for peak metering
    static volatile float peakl,peakr;
    
    
    // if mono, only leftport is used - both will be null if this
    // is a return.
    jack_port_t *leftport,*rightport;
    
    // these cache the port buffers, but only inside one call to process()
    float *left,*right;
    
    void resolveChains(){
        // resolve sends
        for(unsigned int i=0;i<chainNames.size();i++){
            chains[i].chain = ChainInterface::find(chainNames[i]);
        }
        
        // and returns
        if(!leftport) { // we are a return, and returnChainName should be valid
            ChainInterface *ch = ChainInterface::find(returnChainName);
            left = ch->leftoutbuf;
            right = ch->rightoutbuf;
        }
    }
    
    Monitor mon;
    
public:
    Channel(std::string n,int ch,Value *g,Value *p,bool isret,
            std::string rcn="") : mon(n.c_str()){
        name = n;
        mono = ch==1;
        gain = g;
        pan = p;
        returnChainName=rcn;
        
        // if this is a return, we don't create ports - instead,
        // we'll use output buffers in the chains.
        if(isret){
            leftport = rightport = NULL;
        } else {
            if(mono){
                leftport = makePort(n);
                rightport = NULL;
            } else {
                leftport = makePort(n+"_L");
                rightport = makePort(n+"_R");
            }
        }
        if(isret)
            returnchans.push_back(this);
        else
            inputchans.push_back(this);
    }
    
    void addChainInfo(std::string name,Value *v,bool postfade){
        chains.push_back(ChainFeed(v,postfade));
        chainNames.push_back(name);
    }
    
    static void resolveAllChannelChains(){
        std::vector<Channel *>::iterator it;
        for(it=returnchans.begin();it!=returnchans.end();it++){
            (*it)->resolveChains();
        }
        for(it=inputchans.begin();it!=inputchans.end();it++){
            (*it)->resolveChains();
        }
    }
    
    
    // mixes all input channels into the output buffer and into the send
    // buffer, clearing those buffers first. Called *before* effects processing.
    static void mixInputChannels(float *leftout,float *rightout,int offset,int nframes);
    // does the same for the return channels. Called *after* effects processing.
    static void mixReturnChannels(float *leftout,float *rightout,int offset,int nframes);
    
    // call once per process() to cache port buffer pointers. Note
    // this isn't called (or required) for return channels.
    static void cacheAllChannelBuffers(int nframes){
        std::vector<Channel *>::iterator it;
        for(it=inputchans.begin();it!=inputchans.end();it++){
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
