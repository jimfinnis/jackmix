/**
 * @file channel.h
 * @brief  Brief description of file.
 *
 */

#ifndef __CHANNEL_H
#define __CHANNEL_H

#include <jack/jack.h>

// input channel structure

class Channel {
    static const int MAXPORTS=2;
    
    std::string name;
    int chans; // 1 or 2
    
public:
    Channel(std::string n,int ch){
        extern jack_client_t *client;
        if(ch>MAXPORTS || !ch)
            throw "too many chans (or too few)";
        name = n;
        for(int i=0;i<ch;i++){
            port[i] = jack_port_register(
                                         client,
                                         (name+std::to_string(i)).c_str(),
                                         JACK_DEFAULT_AUDIO_TYPE, 
                                         JackPortIsOutput, 0);
        }
        chans = ch;
    }
    
    jack_port_t *port[MAXPORTS];
};
    
    


#endif /* __CHANNEL_H */
