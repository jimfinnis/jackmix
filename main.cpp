/**
 * @file main.cpp
 * @brief  Brief description of file.
 *
 */

#define MAXFRAMESIZE 1024

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

extern void parseConfig(const char *filename);


using namespace std;

jack_port_t *output[2];

/// array of zeroes
float floatZeroes[MAXFRAMESIZE];
/// array of ones
float floatOnes[MAXFRAMESIZE];

/// sample rate 
uint32_t samprate = 0;

jack_client_t *client;

// have to be ptrs so they get added to the list
Value *masterPan,*masterGain;


/*
 * Processing.
 */

// this is called repeatedly by process to do the mixing
inline void subproc(float *left,float *right,int offset,int n){
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

/*
 * JACK callbacks
 */

static volatile bool parsedAndReady=false;

// process thread -> main thread, monitoring data
static RingBuffer<MonitorData> monring(20);
// main thread -> process thread, commands
RingBuffer<MonitorCommand> moncmdring(20);

static void processMonitorCommand(MonitorCommand& c){
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

static PeakMonitor masterMonL("masterL"),masterMonR("masterR");
int process(jack_nframes_t nframes, void *arg){
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
void jack_shutdown(void *arg)
{
    exit(1);
}
int srate(jack_nframes_t nframes, void *arg)
{
    printf("the sample rate is now %" PRIu32 "/sec\n", nframes);
    samprate = nframes;
    return 0;
}

void shutdown(){
    jack_client_close(client);
    exit(0);
}



/*
 * Initialisation
 *
 */

void init(const char *file){
    for(int i=0;i<MAXFRAMESIZE;i++){
        floatZeroes[i]=0.0f;
        floatOnes[i]=1.0f;
    }
    
    masterGain = (new Value())->
          setdb()->setrange(-60,0)->setdef(0)->reset();
    masterPan = (new Value())->
          setdef(0)->setrange(-0.5,0.5)->reset();
    
    initDiamond();
    
    // start JACK
    
    if (!(client = jack_client_open("jackmix", JackNullOption, NULL))){
        throw _("jack not running?");
    }
    // get the real sampling rate
    samprate = jack_get_sample_rate(client);
    
    if(samprate<10000)
        throw _("weird sample rate: %d",samprate);
    
    //    PluginMgr::loadFilesIn("/usr/lib/ladspa");
    PluginMgr::loadFilesIn("./testpl");
    
    // set callbacks
    jack_set_process_callback(client, process, 0);
    jack_set_sample_rate_callback(client, srate, 0);
    jack_on_shutdown(client, jack_shutdown, 0);
    
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
    
    parseConfig(file);
    
    try {
        Ctrl::checkAllCtrlsForSource();
    } catch(const char *s){
        printf("WARN: redundant char* error here\n");
        throw string(s);
    }
    Channel::resolveAllChannelChains();
}

void loop(){
    MonitorUI mon;
    while(1){
        usleep(100000);
        MonitorData mdat;
//        printf("%ld\n",monring.getWriteSpace());
        while(monring.getReadSpace())
            monring.read(mdat);
        pollDiamond();
        
        mon.display(&mdat);
        mon.handleInput();
    }
}

int main(int argc,char *argv[]){
    
    try {
        init(argc>1 ? argv[1] : "config");
    } catch (const char *s){
        printf("Redundant error : %s\n",s);
        exit(1);
    } catch (string s){
        cout << "Fatal error: " << s << endl;
        exit(1);
    }
    
    parsedAndReady=true;
    
    try {
        loop();
    } catch(string s){
        cout << "Fatal error: " << s << endl;
    }
    
    
    shutdown();
}
