/**
 * @file process.cpp
 * @brief  Brief description of file.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <stdint.h>
#include <string>
#include <iostream>

#include "channel.h"
#include "ctrl.h"

#include "exception.h"
#include "tokeniser.h"
#include "tokens.h"
#include "plugins.h"

#include "monitor.h"
#include "diamond.h"

#include "process.h"

using namespace std;

static jack_port_t *output[2];

// statics of Process
volatile bool Process::parsedAndReady=false;
RingBuffer<MonitorData> Process::monring(20);
RingBuffer<MonitorCommand> Process::moncmdring(20);
uint32_t Process::samprate=0;
PeakMonitor Process::masterMonL("masterL"),Process::masterMonR("masterR");
Value *Process::masterPan,*Process::masterGain;
jack_client_t *Process::client=NULL;

void Process::init(){
    masterGain = (new Value())->
          setdb()->setrange(-60,0)->setdef(0)->reset();
    masterPan = (new Value())->
          setdef(0)->setrange(-0.5,0.5)->reset();
}    

void Process::initJack(){
    if (!(client = jack_client_open("jackmix", JackNullOption, NULL))){
        throw _("jack not running?");
    }
    // get the real sampling rate
    samprate = jack_get_sample_rate(client);
    
    if(samprate<10000)
        throw _("weird sample rate: %d",samprate);
    
    // set callbacks
    jack_set_process_callback(client, callbackProcess, 0);
    jack_set_sample_rate_callback(client, callbackSrate, 0);
    jack_on_shutdown(client, callbackShutdown, 0);
    
    output[0] = jack_port_register(
                                   client, 
                                   "out0", 
                                   JACK_DEFAULT_AUDIO_TYPE, 
                                   JackPortIsOutput, 0);
    output[1] = jack_port_register(
                                   client, 
                                   "out1", 
                                   JACK_DEFAULT_AUDIO_TYPE, 
                                   JackPortIsOutput, 0);
    
    if (jack_activate(client)) {
        throw _("cannot activate jack client");
    }
}

void Process::shutdown(){
    jack_client_close(client);
    exit(0);
}

bool Process::pollMonRing(MonitorData *p){
    bool rd=false;
    //        printf("%ld\n",monring.getWriteSpace());
    while(monring.getReadSpace()){
        rd=true;
        monring.read(*p);
    }
    return rd;
}

void Process::writeCmd(MonitorCommandType cmd,
                       float v,class Channel *c){
    if(moncmdring.canWrite()){
        moncmdring.write(MonitorCommand(cmd,v,c));
    }
}    


/*
 * Processing and callbacks
 */

void Process::processMonitorCommand(MonitorCommand& c){
    switch(c.cmd){
    case ChangeGain:
        c.chan->gain->nudge(c.v);
        break;
    case ChangePan:
        c.chan->pan->nudge(c.v);
        break;
    case ChangeMasterGain:
        masterGain->nudge(c.v);
        break;
    case ChangeMasterPan:
        masterPan->nudge(c.v);
        break;
    }
}

int Process::callbackSrate(jack_nframes_t nframes, void *arg)
{
    printf("the sample rate is now %" PRIu32 "/sec\n", nframes);
    Process::samprate = nframes;
    return 0;
}

void Process::callbackShutdown(void *arg){
    exit(1);
}    

void Process::subproc(float *left,float *right,int offset,int n){
    
    // run through all the channels, converting to stereo and panning
    // as required, mixing them into stereo if needed. Add the resulting
    // stereo buffers into the output buffers.
    
    static float tmpl[BUFSIZE],tmpr[BUFSIZE];
    
    ChainInterface::zeroAllInputs();
    // get input channels and mix into buffers (including send chain inputs)
    Channel::mixInputChannels(tmpl,tmpr,offset,n);
    // process effects
    ChainInterface::runAll(n);
    // mix effects return channels into output
    Channel::mixReturnChannels(tmpl,tmpr,offset,n);
    
    // finally set the output
    panstereo(left+offset,right+offset,tmpl,tmpr,
              masterPan->get(),masterGain->get(),n);
}

int Process::callbackProcess(jack_nframes_t nframes, void *arg){
    if(!parsedAndReady)return 0;
    
    // every now and then, read data out of the ring buffers and update
    // the values (which use LPFs).
    Ctrl::pollAllCtrlRings();
    Value::updateAll();
    
    float *outleft = 
          (jack_default_audio_sample_t *)jack_port_get_buffer(output[0],
                                                              nframes);
    float *outright = 
          (jack_default_audio_sample_t *)jack_port_get_buffer(output[1],
                                                              nframes);
    
    // just stores the pointers to the buffers for quick access in processing;
    // they don't survive across process() calls.
    Channel::cacheAllChannelBuffers(nframes);
    
    // we split the buffer into chunks we know are of a certain size
    // to avoid having to play silly buggers with memory allocation
    unsigned int i;
    for(i=0;i<nframes-BUFSIZE;i+=BUFSIZE){
        subproc(outleft+i,outright+i,i,BUFSIZE);
    }
    subproc(outleft+i,outright+i,i,nframes-i);
    
    masterMonL.in(outleft,nframes);
    masterMonR.in(outright,nframes);
    
    
    // write to the monitoring ring buffer
    
    if(monring.getWriteSpace()>3){
        MonitorData m;
        m.master.l = masterMonL.get();
        m.master.r = masterMonR.get();
        m.master.gain = masterGain->get();
        m.master.pan = masterPan->get();
        Channel::writeMons(&m);
        monring.write(m);
    }
    
    // read any commands from the monitor
    MonitorCommand cmd;
    while(moncmdring.getReadSpace()){
        moncmdring.read(cmd);
        processMonitorCommand(cmd);
    }
    
    return 0;
}
