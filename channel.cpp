/**
 * @file channel.cpp
 * @brief  Brief description of file.
 *
 */

#include <string.h>
#include "channel.h"
#include "utils.h"

// two sets of channels - input channels (which have a jack port)
// and return channels (which mix in the output of ladspa effects)

std::vector<Channel *> Channel::inputchans;
std::vector<Channel *> Channel::returnchans;

std::vector<std::string> Channel::chainNames;
std::vector<ChainFeed> Channel::chains;

volatile float Channel::peakl=0;
volatile float Channel::peakr=0;

void Channel::mixInputChannels(float *__restrict leftout,
                               float *__restrict rightout,
                               int offset,int nframes){
    
    memset(leftout,0,nframes*sizeof(float));
    memset(rightout,0,nframes*sizeof(float));
    
    std::vector<Channel *>::iterator it;
    for(it=inputchans.begin();it!=inputchans.end();it++){
        (*it)->mix(leftout,rightout,offset,nframes);
    }
    
    for(int i=0;i<nframes;i++){
        float v = fabs(leftout[i]);
        if(v>peakl)peakl=v;
        v = fabs(rightout[i]);
        if(v>peakr)peakr=v;
    }
    
}

void Channel::mixReturnChannels(float *__restrict leftout,
                                float *__restrict rightout,
                                int offset,int nframes){
    std::vector<Channel *>::iterator it;
    for(it=returnchans.begin();it!=returnchans.end();it++){
        (*it)->mix(leftout,rightout,offset,nframes);
    }
    
    for(int i=0;i<nframes;i++){
        float v = fabs(leftout[i]);
        if(v>peakl)peakl=v;
        v = fabs(rightout[i]);
        if(v>peakr)peakr=v;
    }
}


void Channel::mix(float *__restrict leftout,
                  float *__restrict rightout,int offset,int nframes){
    static float tmpl[BUFSIZE],tmpr[BUFSIZE];
    
    // mix into the temp buffers
    if(mono){
        panmono(tmpl,tmpr,left+offset,
                pan->get(),gain->get(),
                nframes);
    } else {
        panstereo(tmpl,tmpr,left+offset,right+offset,
                pan->get(),gain->get(),
                nframes);
    }
    
    // and add to output buffers
    for(int i=0;i<nframes;i++){
        leftout[i] += tmpl[i];
        rightout[i] += tmpr[i];
    }
    mon.in(tmpl,nframes);
    
    // mix into chains
    std::vector<ChainFeed>::iterator it;
    for(it = chains.begin();it!=chains.end();it++){
        float gain = it->gain->get();
        if(it->postfade){
            if(mono)
                it->chain->addMono(tmpl,gain);
            else
                it->chain->addStereo(tmpl,tmpr,gain);
        } else {
            if(mono)
                it->chain->addMono(left+offset,gain);
            else
                it->chain->addStereo(left+offset,right+offset,gain);
        }
    }
    
}
