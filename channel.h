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
    static const int MAXPORTS=4;
    
    std::string name;
    int chans; // up to 4, any more is a bit silly.
    
public:
    Channel(std::string n,int ch){
        extern jack_client_t *client;
        if(ch>MAXPORTS)
            throw "too many chans";
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
