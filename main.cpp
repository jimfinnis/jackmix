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

#include "channel.h"
#include "ctrl.h"

#include "exception.h"
#include "tokeniser.h"
#include "tokens.h"
#include "plugins.h"

#include "monitor.h"
#include "diamond.h"

#include "process.h"

extern void parseConfig(const char *filename);



/*
 * Initialisation
 *
 */


void loop(){
    MonitorUI mon;
    while(1){
        usleep(100000);
        
        static MonitorData mdat;
        if(Process::pollMonRing(&mdat))
            mon.display(&mdat);
        
        pollDiamond();
        mon.handleInput();
    }
}

void shutdown(int s){
    throw _("abort");
}

int main(int argc,char *argv[]){
    
    signal(SIGINT,shutdown);
    signal(SIGQUIT,shutdown);
    
    try {
        // initialise data structures
        Process::init();
        // initialise comms
        initDiamond();
        // initialise Jack
        Process::initJack();
        
        // load LADSPA plugins
        PluginMgr::loadFilesIn("/usr/lib/ladspa");
        //PluginMgr::loadFilesIn("./testpl");
        
        // parse the config
        parseConfig("config");
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
        loop();
    } catch(string s){
        //        PluginMgr::close();
        Process::shutdown();
        cout << "Fatal error: " << s << endl;
    }
    Process::shutdown();
}
