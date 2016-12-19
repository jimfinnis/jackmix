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

#include "channel.h"
#include "ctrl.h"

#include "tokeniser.h"
#include "tokens.h"

static const float PI = 3.1415927f;

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

// mono panning using a sin/cos taper to avoid the 6db drop at the centre
void panmono(float *__restrict left,
             float *__restrict right,
             float *__restrict in,
             float pan,float amp,int n){
    float ampl = cosf(pan*PI*0.5f);
    float ampr = sinf(pan*PI*0.5f);
    for(int i=0;i<n;i++){
        *left++ = *in * ampl;
        *right++ = *in++ * ampr;
    }
}

// stereo panning (balance) using a linear taper
void panstereo(float *__restrict leftout,
               float *__restrict rightout,
               float *__restrict leftin,
               float *__restrict rightin,
               float pan,float amp,int n){
    if(pan<0.5f){
        for(int i=0;i<n;i++){
            *leftout++ = *leftin++;
            *rightout++ = *rightin++ * pan*2.0f;
        }
    } else {
        pan = 1.0f-pan;
        for(int i=0;i<n;i++){
            *leftout++ = *leftin++ * pan*2.0f;
            *rightout++ = *rightin++;
        }
    }
        
}


static const unsigned int BUFSIZE=1024;

void subproc(float *left,float *right,int n){
    static float tmp[BUFSIZE];
    
}

/*
 * JACK callbacks
 */


int process(jack_nframes_t nframes, void *arg){
    float *outleft = 
          (jack_default_audio_sample_t *)jack_port_get_buffer(output[0],
                                                              nframes);
    float *outright = 
          (jack_default_audio_sample_t *)jack_port_get_buffer(output[1],
                                                              nframes);
    
    static float buf[BUFSIZE];
    unsigned int i;
    for(i=0;i<nframes-BUFSIZE;i+=BUFSIZE){
        subproc(outleft+i,outright+i,BUFSIZE);
    }
    subproc(outleft+i,outright+i,nframes-i);
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
        throw "syntax error";
    return std::string(tok.getstring());
}

// values are <number>['('<ctrl>')']
Value *parseValue(){
    float n = tok.getnextfloat();
    if(tok.iserror())throw "expected a number";
    
    Value *v = new Value();
    v->setdef(n);
    
    if(tok.getnext()==T_OPREN){
        // there is a controller for this value!
        std::string name = getnextident();
        Ctrl *c = Ctrl::createOrFind(name);
        c->addval(v);
        if(tok.getnext()!=T_CPREN)
            throw "expected a ')'";
    } else
        tok.rewind();
}
    

void parseChan(){
    if(tok.getnext()!=T_COLON)
        throw "expected ':'";
    std::string s=getnextident();

}

void parse(const char *s){
    tok.reset(s);
    for(;;){
        switch(tok.getnext()){
        case T_CHANS:
            parseChan();
            break;
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
    
    // line by line parsing
    try {
        tok.init();
        tok.setname("<in>");
        tok.settokens(tokens);
        tok.setcommentlinesequence("#");
        
        FILE *a = fopen(file,"r");
        if(a){
            while(!feof(a)){
                char buf[1024];
                char *bb = fgets(buf,1024,a);
                if(bb)parse(bb);
            }
            fclose(a);
        }
    } catch(const char *s){
        return s;
    }
    
    
    
    return NULL;
}

int main(int argc,char *argv[]){
    if(const char *err = init("config")){
        printf("fatal error: %s\n",err);
        exit(1);
    }
    sleep(10);
    shutdown();
}
