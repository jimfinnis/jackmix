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
#include "parser.h"
#include "monitor.h"
#include "diamond.h"

using namespace std;

jack_port_t *output[2];

/// array of zeroes
float floatZeroes[MAXFRAMESIZE];
/// array of ones
float floatOnes[MAXFRAMESIZE];

/// sample rate 
uint32_t samprate = 0;

jack_client_t *client;

/*
 * Processing.
 */

// this is called repeatedly by process to do the mixing
inline void subproc(float *left,float *right,int offset,int n){
    // run through all the channels, converting to stereo and panning
    // as required, mixing them into stereo if needed. Add the resulting
    // stereo buffers into the output buffers.
    
    ChainInterface::zeroAllInputs();
    // get input channels and mix into buffers (including send chain inputs)
    Channel::mixInputChannels(left,right,offset,n);
    // process effects
    ChainInterface::runAll(n);
    // mix effects return channels into output
    Channel::mixReturnChannels(left,right,offset,n);
}

/*
 * JACK callbacks
 */

static volatile bool parsedAndReady=false;
static RingBuffer<MonitorData> monring(20);
RingBuffer<MonitorCommand> moncmdring(20);

static void processMonitorCommand(MonitorCommand& c){
    switch(c.cmd){
    case ChangeGain:
        c.chan->gain->nudge(c.v);
        break;
    case ChangePan:
        c.chan->pan->nudge(c.v);
        break;
    }
}

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
    
    // write to the monitoring ring buffer
    
    if(monring.getWriteSpace()>3){
        MonitorData m;
        m.master.l = Channel::getPeakL();
        m.master.r = Channel::getPeakR();
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
 * 
 * Parser
 *
 */

Tokeniser tok;

// values are <number>['('<ctrl>')'], where <number> might be "default"
// bounds: a Bounds structure containing an upper and/or lower bound,
// which cannot be overriden with min/max (used in effects).
Value *parseValue(Bounds b)
{
    Value *v = new Value();
    float rmin=0,rmax=1;
    float smooth = 0.5;
    
    for(;;){
        switch(tok.getnext()){
        case T_DB:
            v->setdb();
            // might get overwritten later..
            rmin=-60;rmax=0;
            break;
        case T_MIN:
            if(b.flags & Bounds::Lower)
                throw _("'min' not permitted, min is fixed to %f",b.lower);
            rmin = getnextfloat();
            break;
        case T_MAX:
            if(b.flags & Bounds::Upper)
                throw _("'max' not permitted, max is fixed to %f",b.upper);
            rmax = getnextfloat();
            break;
        case T_SMOOTH:
            smooth = tok.getnextfloat();
            if(tok.iserror())expected("number");
            break;
        case T_DEFAULT:
            if(!(b.flags & Bounds::Default))
                throw _("no default is provided for this value");
        case T_INT:
        case T_FLOAT:
            goto optsdone;
        default:
            expected("number or value option");
        }
    }
    
optsdone:
    
    if(b.flags & Bounds::Upper)
        rmax = b.upper;
    if(b.flags & Bounds::Lower)
        rmin = b.lower;
    
    // use the default value passed in if we have one and it's allowed
    float n;
    if(tok.getcurrent() == T_DEFAULT){ // checked above
        n = b.deflt;
    } else if(tok.getcurrent() == T_FLOAT || tok.getcurrent()==T_INT)
        n = tok.getfloat();
    else
        expected("number or 'default'");
    
    v->setdef(n);
    v->setrange(rmin,rmax);
    v->reset();
    
    if(tok.getnext()==T_OPREN){
        // there is a controller for this value!
        string name = getnextident();
        Ctrl *c = Ctrl::createOrFind(name);
        c->addval(v);
        
        if(tok.getnext()==T_SMOOTH){
            n = tok.getnextfloat();
            if(tok.iserror())expected("number");
        } else tok.rewind();
        
        if(tok.getnext()!=T_CPREN)
            expected("')'");
    } else
        tok.rewind();
    v->setsmooth(smooth);
    return v;
}


void parseChan(){
    string name=getnextident();
    
    if(tok.getnext()!=T_COLON)
        expected(":");
    
    printf("Parsing channel %s\n",name.c_str());
    
    string returnChainName;
    bool isReturn=false;
    if(tok.getnext()==T_RETURN){
        isReturn=true;
        returnChainName = getnextident();
    }else tok.rewind();
    
    if(tok.getnext()!=T_GAIN)
        expected("'gain'");
    Value *gain = parseValue(Bounds());
    
    if(tok.getnext()!=T_PAN)
        expected("'pan'");
    Value *pan = parseValue(Bounds());
    
    bool mono=false;
    switch(tok.getnext()){
    case T_MONO:mono=true;
    case T_STEREO:break;
    default:tok.rewind();break;
    }
    
    // can now create the channel. The Channel class maintains
    // a static list of channels to which the ctor will add the
    // new one.
    Channel *ch = new Channel(name,mono?1:2,gain,pan,isReturn,
                              returnChainName);
    
    // add info about chains, which will be resolved later.
    while(tok.getnext()==T_SEND){
        string chain = getnextident();
        if(tok.getnext()!=T_GAIN)expected("'gain'");
        Value *chaingain=parseValue(Bounds());
        
        int t = tok.getnext();
        bool postfade=false;
        if(t==T_POSTFADE)postfade=true;
        else if(t!=T_PREFADE)tok.rewind();
        
        ch->addChainInfo(chain,chaingain,postfade);
    }
    tok.rewind(); // should leave us at a comma
    
    
}

void parseCtrl(){
    string name=getnextident();
    
    if(tok.getnext()!=T_COLON)
        expected("':' after ctrl name");
    
    if(tok.getnext()!=T_STRING)
        expected("string (ctrl source spec)");
    
    string spec = string(tok.getstring());
    
    // creating the ctrl will set the default ranges
    
    Ctrl *c = Ctrl::createOrFind(name);
    c->setsource(spec);
    
    // get the in-range, which converts to 0-1.
    
    if(tok.getnext()!=T_IN)expected("'in'");
    
    float inmin = tok.getnextfloat();
    if(tok.iserror())expected("float (in input range)");
    if(tok.getnext()!=T_COLON)expected("':' in input range");
    float inmax = tok.getnextfloat();
    if(tok.iserror())expected("float (in input range)");
    c->setinrange(inmin,inmax);
    
}

// parse plugin data: currently just shortnames for ports
void parsePlugin(){
    string name=getnextident();
    PluginData *p = PluginMgr::getPlugin(name);
    if(tok.getnext()!=T_OCURLY)expected("'{'");
    
    if(tok.getnext()==T_NAMES){
        parseList([=]{
                  string sn = getnextident();
                  string ln = getnextidentorstring();
                  p->addShortPortName(sn,ln);
              });
    } else tok.rewind();
    
    if(tok.getnext()!=T_CCURLY)expected("'}'");
    
}


void parse(const char *s){
    extern void parseStereoChain();
    tok.reset(s);
    for(;;){
        switch(tok.getnext()){
        case T_CHANS:
            parseList(parseChan);
            break;
        case T_CTRL:
            parseList(parseCtrl);
            break;
        case T_CHAIN:
            parseList(parseStereoChain);
            break;
        case T_PLUGINS:
            parseList(parsePlugin);
            break;
        case T_END:
            return;
        default:
            expected("chans, ctrls, fx, plugins");
        }
    }
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
    
    // parsing - reads entire file
    try {
        tok.init();
        tok.setname("<in>");
        tok.settokens(tokens);
        tok.setcommentlinesequence("#");
        //        tok.settrace(true);
        FILE *a = fopen(file,"r");
        if(a){
            fseek(a,0L,SEEK_END);
            int n = ftell(a);
            fseek(a,0L,SEEK_SET);
            char *fbuf = (char *)malloc(n+1);
            fread(fbuf,sizeof(char),n,a);
            fbuf[n]=0;
            fclose(a);
            parse(fbuf);
            free(fbuf);
            
        } else 
            throw _("cannot open config file");
    } catch(string s){
        stringstream ss;
        ss << "at line " << tok.getline() << ": " << s;
        throw ss.str();
    }
    
    
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
        //        printf("%f - %f\n",Channel::getPeakL(),Channel::getPeakR());
        Channel::resetPeak();
        
        mon.display(&mdat);
        mon.handleInput();
    }
}

int main(int argc,char *argv[]){
    
    try {
        init("config");
    } catch (const char *s){
        printf("Redundant error : %s\n",s);
        exit(1);
    } catch (string s){
        cout << "Fatal error: " << s << endl;
        exit(1);
    }
    
    Channel::resetPeak();
    parsedAndReady=true;
    
    try {
        loop();
    } catch(string s){
        cout << "Fatal error: " << s << endl;
    }
    
    
    shutdown();
}
