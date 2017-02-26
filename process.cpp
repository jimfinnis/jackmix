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
#include "fx.h"

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
RingBuffer<ProcessCommand> Process::moncmdring(20);
RingBuffer<ProcessCommand> sendqueue(20);

uint32_t Process::samprate=0;
PeakMonitor Process::masterMonL("masterL"),Process::masterMonR("masterR");
Value *Process::masterPan,*Process::masterGain;
jack_client_t *Process::client=NULL;

// used to lock the UI while the process does its stuff
static pthread_mutex_t cmdmutex = PTHREAD_MUTEX_INITIALIZER; 
static pthread_cond_t cmdcond = PTHREAD_COND_INITIALIZER;

void Process::init(){
    masterGain = (new Value())->
          setdb()->setrange(-60,0)->setdef(0)->reset();
    masterPan = (new Value())->
          setdef(0)->setrange(0,1)->setdef(0.5)->reset();
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
    jack_deactivate(client);usleep(10000);
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


void Process::writeCmd(ProcessCommand cmd){
    // queue a command
    if(sendqueue.canWrite())
        sendqueue.write(cmd);
}

void Process::sendCmds(){
    while(sendqueue.getReadSpace()){
        ProcessCommand cmd;
        sendqueue.read(cmd);
        if(moncmdring.canWrite()){
            moncmdring.write(cmd);
        }
    }
    // block until commands done
    pthread_cond_wait(&cmdcond,&cmdmutex);
}


/*
 * Processing and callbacks
 */

void Process::processCommand(ProcessCommand& c){
    switch(c.cmd){
    case NudgeValue:
        c.vp->nudge(c.v);
        break;
    case SetValue:
        c.vp->setTarget(c.v);
        break;
    case ChannelMute:
        c.chan->toggleMute();
        break;
    case ChannelSolo:
        c.chan->toggleSolo();
        break;
    case DelSend:
        // awkward. 
        c.chan->removeChainInfo(c.arg0);
        break;
    case DelChan:
        delete c.chan; // should remove from lists.
        break;
    case AddSend:{
        // Will leave a dangling value. No biggie.
        Value *v = new Value();
        v->setdb()->setrange(-60,0)->setdef(0)->reset();
        c.chan->addChainInfo(c.s,v,false,ChainInterface::find(c.s));
        break;
    }
    case AddChannel:{
        // Will leave 2 dangling values. Bit more biggie.
        Value *g = new Value();
        g->setdb()->setrange(-60,0)->setdef(0)->reset();
        Value *p = new Value();
        p->setrange(0,1)->setdef(0.5)->reset();
        new Channel(c.s,c.arg0,g,p,false);
    } break;
    case TogglePrePost:
        c.chan->chains[c.arg0].postfade=
              !c.chan->chains[c.arg0].postfade;
        break;
    case AddEffect:{
        ChainInterface *ch = chainlist[c.arg0];
        ch->addEffect(c.pld,c.s);
    } break;
    case RemapInput:{
        ChainInterface *ch = chainlist[c.arg0];
        ch->remapInput(c.s,c.s2,c.arg1,c.s3,c.s4);
    } break;
    case RemapOutput:{
        ChainInterface *ch = chainlist[c.arg0];
        ch->remapOutput(c.arg1,c.s,c.s2);
        break;
    }
    case AddChain:
        ChainInterface::addNewEmptyChain(c.s);
        break;
    case DeleteChain:
        ChainInterface::deleteChain(c.arg0);
        break;
    case DeleteEffect:
        ChainInterface::deleteEffect(c.arg0,c.arg1);
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
    ProcessCommand cmd;
    while(moncmdring.getReadSpace()){
        moncmdring.read(cmd);
        processCommand(cmd);
    }
    pthread_cond_signal(&cmdcond);
    
    return 0;
}

