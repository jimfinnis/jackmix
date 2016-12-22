/**
 * @file channel.cpp
 * @brief  Brief description of file.
 *
 */

#include <string.h>
#include "channel.h"
#include "utils.h"

std::vector<Channel *> Channel::chans;
volatile float Channel::peakl=0;
volatile float Channel::peakr=0;

void Channel::mixChannels(float *__restrict leftout,
                          float *__restrict rightout,
                          int offset,int nframes){
    
    memset(leftout,0,nframes*sizeof(float));
    memset(rightout,0,nframes*sizeof(float));
    
    std::vector<Channel *>::iterator it;
    for(it=chans.begin();it!=chans.end();it++){
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
}
