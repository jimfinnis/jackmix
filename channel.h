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
#include <fstream>
#include "value.h"
#include "global.h"
#include "fx.h"

// info describing how a chain is fed
struct ChainFeed {
    // from the parser, ctor will just sets up gain and postfade,
    // chain is set later. In the monitor, chain is set up too.
    ChainFeed(Value *v,bool pf,ChainInterface *c){
        gain = v;
        postfade = pf;
        chain = c;
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
    
    static std::vector<Channel *> inputchans,returnchans;
    
    
    
    // create an input port
    static jack_port_t *makePort(std::string pname);
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
    
    PeakMonitor monl,monr;
    bool mute=false;
    static Channel *solochan;
    
public:
    Value *pan,*gain;
    // names of chains, same indexing as "chains"
    std::vector<std::string> chainNames;
    // pointers to actual chains, built from chainNames after parsing.
    std::vector<ChainFeed> chains;
    
    void toggleMute(){
        mute = !mute;
    }
    
    void toggleSolo(){
        if(solochan == this)
            solochan = NULL;
        else
            solochan = this;
    }
    
    bool isMute(){
        return mute;
    }
    
    bool isSolo(){
        return solochan == this;
    }
        
    
    Channel(std::string n,int ch,Value *g,Value *p,bool isret,
            std::string rcn="") : monl(n+"l"), monr(n+"r"){
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
    
    // add a chain send - if from the parser, leave chain NULL to be resolved.
    void addChainInfo(std::string name,Value *v,bool postfade,ChainInterface *c=NULL){
        chains.push_back(ChainFeed(v,postfade,c));
        chainNames.push_back(name);
    }
    
    // remove a chain send or nothing if there is no such send.
    // DO NOT CALL FROM MAIN THREAD.
    void removeChainInfo(unsigned int i){
        if(i<chainNames.size()){
            chainNames.erase(chainNames.begin()+i);
            chains.erase(chains.begin()+i);
        }
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
    
    static void writeMons(struct MonitorData* m);
    
    
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
    
    // save a single channel 
    void save(std::ostream& out);
    
    // save all channels
    static void saveAll(std::ostream& out);
    
};
    
    


#endif /* __CHANNEL_H */
