/**
 * @file main.cpp
 * @brief  Brief description of file.
 *
 */


#include <jack/jack.h>

jack_port_t *output_port;

/// table of midi note "frequencies" adjusted by sample rate
jack_default_audio_sample_t note_frqs[128]; 

/// array of zeroes
float floatZeroes[MAXFRAMESIZE];
/// array of ones
float floatOnes[MAXFRAMESIZE];

/// sample rate 
float samprate = 0;
/// frequency of note being played
float keyFreq = 440.0;

/// various other globals
jack_default_audio_sample_t amp=0.0;

jack_client_t *client;

/*
 * JACK callbacks
 */

int process(jack_nframes_t nframes, void *arg){
    /// the actual synth currently running
    static NoteCmd *curcmd=NULL;
    
    jack_default_audio_sample_t *out = 
          (jack_default_audio_sample_t *)jack_port_get_buffer(output_port,
                                                              nframes);
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

void init(){
    for(int i=0;i<MAXFRAMESIZE;i++){
        floatZeroes[i]=0.0f;
        floatOnes[i]=1.0f;
    }
    
    // start JACK
    
    if (!(client = jack_client_open("sonicaes", JackNullOption, NULL))){
        throw "jack not running?";
    }
    // get the real sampling rate
    samprate = (float)jack_get_sample_rate(client);

    // set callbacks
    jack_set_process_callback(client, process, 0);
    jack_set_sample_rate_callback(client, srate, 0);
    jack_on_shutdown(client, jack_shutdown, 0);
    
    output_port = jack_port_register(
                                     client, 
                                     "out", 
                                     JACK_DEFAULT_AUDIO_TYPE, 
                                     JackPortIsOutput, 0);
    
    if (jack_activate(client)) {
        throw "cannot activate jack client");
    }
}

int main(int argc,char *argv[]){
    init();
    sleep(10);
    shutdown();
}
