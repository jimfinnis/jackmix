/**
 * @file channel.cpp
 * @brief  Brief description of file.
 *
 */

#include <sstream>
#include <string.h>
#include "channel.h"
#include "utils.h"
#include "monitor.h"
#include "save.h"
#include "process.h"
#include "ctrl.h"

Channel *Channel::solochan=NULL;

Channel::~Channel(){
    // remove the channel's values from the controlled values
    Ctrl::removeAllAssociations(gain);
    Ctrl::removeAllAssociations(pan);
    
    // remove the channel from the appropriate list
    std::vector<Channel *> &vec = leftport ? inputchans : returnchans;
    // and apparently C++ is a *good* language?
    vec.erase(std::remove(vec.begin(),vec.end(),this),vec.end());
    
    // and now delete the ports
    if(leftport)
        jack_port_unregister(Process::client,leftport);
    if(rightport)
        jack_port_unregister(Process::client,rightport);
          
}
    
    


// two sets of channels - input channels (which have a jack port)
// and return channels (which mix in the output of ladspa effects)

jack_port_t *Channel::makePort(std::string pname){
    return jack_port_register(
                              Process::client,
                              pname.c_str(),
                              JACK_DEFAULT_AUDIO_TYPE, 
                              JackPortIsInput, 0);
}

std::vector<Channel *> Channel::inputchans;
std::vector<Channel *> Channel::returnchans;

void Channel::mixInputChannels(float *__restrict leftout,
                               float *__restrict rightout,
                               int offset,int nframes){
    
    memset(leftout,0,nframes*sizeof(float));
    memset(rightout,0,nframes*sizeof(float));
    
    std::vector<Channel *>::iterator it;
    for(it=inputchans.begin();it!=inputchans.end();it++){
        (*it)->mix(leftout,rightout,offset,nframes);
    }
}

void Channel::mixReturnChannels(float *__restrict leftout,
                                float *__restrict rightout,
                                int offset,int nframes){
    std::vector<Channel *>::iterator it;
    for(it=returnchans.begin();it!=returnchans.end();it++){
        (*it)->mix(leftout,rightout,offset,nframes);
    }
}

void Channel::writeMons(MonitorData *m){
    std::vector<Channel *>::iterator it;
    for(it=inputchans.begin();it!=inputchans.end();it++){
        Channel *c= (*it);
        ChanMonData *cm = m->add();
        if(cm){
            cm->init(c->name,
                     c->monl.get(),c->monr.get(),
                     c->gain->getNoDBConvert(),c->pan->get(),c);
        }
    }
    for(it=returnchans.begin();it!=returnchans.end();it++){
        Channel *c= (*it);
        ChanMonData *cm = m->add();
        if(cm){
            cm->init(c->name,
                     c->monl.get(),c->monr.get(),
                     c->gain->getNoDBConvert(),c->pan->get(),c);
        }
    }
}


void Channel::mix(float *__restrict leftout,
                  float *__restrict rightout,int offset,int nframes){
    static float tmpl[BUFSIZE],tmpr[BUFSIZE];
    
    // skip any channels without the required buffers - i.e. return chans from
    // chains with no FX.
    if(!left || (!mono && !right))
        return;
    
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
    
    // monitoring
    monl.in(tmpl,nframes);
    monr.in(tmpr,nframes);
    
    // add the channel to the master outputs if it is not muted,
    // and there is not a solo channel (which isn't us)
    if(!mute && !(solochan && (this!=solochan))){
        // and add to output buffers
        for(int i=0;i<nframes;i++){
            leftout[i] += tmpl[i];
            rightout[i] += tmpr[i];
        }
    }
    
    // mix into chains
    std::vector<ChainFeed>::iterator it;
    for(it = chains.begin();it!=chains.end();it++){
        float gain = it->gain->get();
        if(it->postfade){
            if(mono)
                it->chain->addMono(tmpl,gain);
            else
                it->chain->addStereo(tmpl,tmpr,gain);
        } else {
            if(mono)
                it->chain->addMono(left+offset,gain);
            else
                it->chain->addStereo(left+offset,right+offset,gain);
        }
    }
}

void Channel::removeReturnChannelsAndSends(std::string chainname){
    // remove sends
    for(unsigned int i=0;i<inputchans.size();i++){
        inputchans[i]->removeChainInfo(chainname);
    }
    for(unsigned int i=0;i<returnchans.size();i++){
        // unlikely, this, because we don't often have a return with
        // a send. But we might.
        returnchans[i]->removeChainInfo(chainname);
    }
    // finally, remove the return channels
    vector<Channel *>::iterator it = returnchans.begin();
    while(it!=returnchans.end()){
        Channel *c = *it;
        if(c->isReturn() && c->getReturnName()==chainname){
            it = returnchans.erase(it);
        } else {
            it++;
        }
    }
    
}


void Channel::save(ostream& out){
    out << "  " << name << ": ";
    if(!leftport && !rightport) { // must be an fx return
        out << "return " << returnChainName << "\n    ";
    }
    out << "gain " << gain->toString() <<endl;
    out << "    pan " << pan->toString();
    
    out << "\n    " << (mono?"mono":"stereo");
    
    for(unsigned int i=0;i<chains.size();i++){
        out << "\n    send " << chainNames[i];
        out << " gain " << chains[i].gain->toString();
        out << (chains[i].postfade ? " postfade " : " prefade ");
    }
    
    
    // does not terminate with NL because of possibility of comma
}

void Channel::saveAll(ostream& out){
    out << "chans {\n";
    
    vector<string> chanstrs;
    for(unsigned int i=0;i<inputchans.size();i++){
        stringstream ss;
        inputchans[i]->save(ss);
        chanstrs.push_back(ss.str());
    }
    for(unsigned int i=0;i<returnchans.size();i++){
        stringstream ss;
        returnchans[i]->save(ss);
        chanstrs.push_back(ss.str());
    }
    
    out << intercalate(chanstrs,",\n\n");
              
    out << "\n}\n";
}
