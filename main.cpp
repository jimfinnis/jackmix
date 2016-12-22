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
#include <string>
#include <iostream>

#include "channel.h"
#include "ctrl.h"

#include "tokeniser.h"
#include "tokens.h"
#include "diamond.h"

jack_port_t *output[2];

/// array of zeroes
float floatZeroes[MAXFRAMESIZE];
/// array of ones
float floatOnes[MAXFRAMESIZE];

/// sample rate 
float samprate = 0;

jack_client_t *client;

/*
 * Processing.
 */

// this is called repeatedly by process to do the mixing
inline void subproc(float *left,float *right,int offset,int n){
    // run through all the channels, converting to stereo and panning
    // as required, mixing them into stereo if needed. Add the resulting
    // stereo buffers into the output buffers.
    
    Channel::mixChannels(left,right,offset,n);
}

/*
 * JACK callbacks
 */

static volatile bool parsedAndReady=false;
int process(jack_nframes_t nframes, void *arg){
    static unsigned int interval=0;
    
    if(!parsedAndReady)return 0;
    float *outleft = 
          (jack_default_audio_sample_t *)jack_port_get_buffer(output[0],
                                                              nframes);
    float *outright = 
          (jack_default_audio_sample_t *)jack_port_get_buffer(output[1],
                                                              nframes);
    
    // every now and then, read data out of the ring buffers and update
    // the values (which use LPFs).
    if(!(interval++%16)){
        Ctrl::pollAllCtrlRings();
        Value::updateAll();
    }
    
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
std::string getnextident(){
    if(tok.getnext()!=T_IDENT)
        throw "expected identifier";
    return std::string(tok.getstring());
}

// values are <number>['('<ctrl>')']
Value *parseValue(){
    float n = tok.getnextfloat();
    if(tok.iserror())throw "expected a number";
    
    Value *v = new Value();
    v->setdef(n);
    
    if(tok.getnext()==T_DB)
        v->setdb();
    else
        tok.rewind();
    v->reset();
    
    if(tok.getnext()==T_OPREN){
        // there is a controller for this value!
        std::string name = getnextident();
        Ctrl *c = Ctrl::createOrFind(name);
        c->addval(v);
        if(tok.getnext()!=T_CPREN)
            throw "expected a ')'";
    } else
        tok.rewind();
    return v;
}


void parseChan(){
    std::string name=getnextident();
    
    printf("Parsing channel %s\n",name.c_str());
    
    
    if(tok.getnext()!=T_GAIN)
        throw "expected 'gain'";
    Value *gain = parseValue();
    
    if(tok.getnext()!=T_PAN)
        throw "expected 'pan'";
    Value *pan = parseValue();
    
    bool mono=false;
    switch(tok.getnext()){
    case T_MONO:mono=true;
    case T_STEREO:break;
    default:tok.rewind();break;
    }
    
    if(tok.getnext()==T_SEND){
        throw "FX not implemented yet";
    } else tok.rewind();
    
    // can now create the channel. The Channel class maintains
    // a static list of channels to which the ctor will add the
    // new one.
    new Channel(name,mono?1:2,gain,pan);
    
}

void parseCtrl(){
    std::string name=getnextident();
    
    if(tok.getnext()!=T_COLON)
        throw "expected ':' after ctrl name";
    
    if(tok.getnext()!=T_STRING)
        throw "expected string (ctrl source spec)";
    
    std::string spec = std::string(tok.getstring());
    
    // creating the ctrl will set the default ranges
    
    Ctrl *c = Ctrl::createOrFind(name);
    c->setsource(spec);
    
    if(tok.getnext()==T_NOCONVERT){
        c->noconvert();
    } else {
        tok.rewind();
        if(tok.getnext()==T_IN){
            float inmin = tok.getnextfloat();
            if(tok.iserror())throw "expected a float in input range";
            if(tok.getnext()!=T_COLON)throw "expected a ':' in input range";
            float inmax = tok.getnextfloat();
            if(tok.iserror())throw "expected a float in input range";
            c->setinrange(inmin,inmax);
        }else tok.rewind();
        
        if(tok.getnext()==T_OUT){
            float outmin = tok.getnextfloat();
            if(tok.iserror())throw "expected a float in output range";
            if(tok.getnext()!=T_COLON)throw "expected a ':' in output range";
            float outmax = tok.getnextfloat();
            if(tok.iserror())throw "expected a float in output range";
            c->setoutrange(outmin,outmax);
        }else tok.rewind();
    }
    
}

void parseChanList(){
    if(tok.getnext()!=T_OCURLY)
        throw("expected '{'");
    for(;;){
        parseChan();
        int t = tok.getnext();
        if(t==T_CCURLY)break;
        else if(t!=T_COMMA)throw("expected ',' or '{'");
    }
}

void parseCtrlList(){
    if(tok.getnext()!=T_OCURLY)
        throw("expected '{'");
    for(;;){
        parseCtrl();
        int t = tok.getnext();
        if(t==T_CCURLY)break;
        else if(t!=T_COMMA)throw("expected ',' or '{'");
    }
}


void parse(const char *s){
    tok.reset(s);
    for(;;){
        switch(tok.getnext()){
        case T_CHANS:
            parseChanList();
            break;
        case T_CTRL:
            parseCtrlList();
            break;
        case T_END:
            return;
        }
    }
}


/*
 * Initialisation
 *
 */

const char *init(const char *file){
    for(int i=0;i<MAXFRAMESIZE;i++){
        floatZeroes[i]=0.0f;
        floatOnes[i]=1.0f;
    }
    
    initDiamond();
    
    // start JACK
    
    if (!(client = jack_client_open("jackmix", JackNullOption, NULL))){
        return "jack not running?";
    }
    // get the real sampling rate
    samprate = (float)jack_get_sample_rate(client);
    
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
        return "cannot activate jack client";
    }
    
    // parsing - reads entire file
    try {
        tok.init();
        tok.setname("<in>");
        tok.settokens(tokens);
        tok.setcommentlinesequence("#");
        tok.settrace(true);
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
            throw "cannot open config file";
    } catch(const char *s){
        static char buf[1024];
        sprintf(buf,"Fatal error at line %d: %s\n",tok.getline(),s);
        return buf;
    }
    try {
        Ctrl::checkAllCtrlsForSource();
    } catch(const char *s){
        static char buf[1024];
        sprintf(buf,"Fatal error: %s\n",s);
        return buf;
    }
    
    return NULL;
}

int main(int argc,char *argv[]){
    
    if(const char *err = init("config")){
        printf("fatal error: %s\n",err);
        exit(1);
    }
    
    Channel::resetPeak();
    parsedAndReady=true;
    while(1){
        usleep(100000);
        pollDiamond();
        printf("%f - %f\n",Channel::getPeakL(),Channel::getPeakR());
        Channel::resetPeak();
    }
    
    shutdown();
}
