/**
 * @file main.cpp
 * @brief  Brief description of file.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <stdint.h>
#include <signal.h>
#include <string>
#include <iostream>
#include <getopt.h>
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

// command line options
struct option opts[]={
    {"nogui",no_argument,NULL,'n'},
    {NULL,0,NULL,0}
};

extern void parseConfig(const char *filename);



/*
 * Initialisation
 *
 */

void poll(){
    pollDiamond();
}
    

void noguiloop(){
    while(1){
        usleep(100000);
        static MonitorData mdat;
        Process::pollMonRing(&mdat);
        poll();
    }
}


void guiloop(){
    // start the non-blocking monitor thread.
    MonitorThread thread;
    InputManager input;
    
    // run the screens until we complete, when the thread will die.
    input.flow();
}

void usage(){
    cerr << "usage:\n"
          << "jackmix [-n] [configfile]\n";
}

int main(int argc,char *argv[]){
    
    bool nogui=false;
    
    try {
        const char *filename="config";
        for(;;){
            int optind=0;
            char c = getopt_long(argc,argv,"n",opts,&optind);
            if(c<0)break;
            switch(c){
            case 'n':
                nogui=true;
                break;
            default:
                usage();
                throw _("incorrect usage");
            }
        }
        if(optind<argc)
            filename = argv[optind];
        // initialise data structures
        Process::init();
        // initialise comms
        initDiamond();
        // initialise Jack
        Process::initJack();
        
        // load LADSPA plugins
        PluginMgr::loadFilesIn("/usr/lib/ladspa",true);
        //PluginMgr::loadFilesIn("./testpl");
        
        // parse the config
        parseConfig(filename);
        Ctrl::checkAllCtrlsForSource();
        Channel::resolveAllChannelChains();
    } catch (const char *s){
        printf("Redundant error : %s\n",s);
        exit(1);
    } catch (string s){
        cout << "Fatal error: " << s << endl;
        exit(1);
    }
    
    // start the processing thread
    Process::parsedAndReady=true;
    
    try {
        if(nogui)
            noguiloop();
        else {
            // start the non-blocking monitor thread
            guiloop();
        }
        
    } catch(string s){
        //        PluginMgr::close();
        Process::shutdown();
        cout << "Fatal error: " << s << endl;
    }
    Process::shutdown();
}
