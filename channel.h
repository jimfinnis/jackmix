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
                                  JackPortIsOutput, 0);
    }
    
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
    
    // if mono, only leftport is used
    jack_port_t *leftport,*rightport;
};
    
    


#endif /* __CHANNEL_H */
