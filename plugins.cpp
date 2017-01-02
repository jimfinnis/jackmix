/**
 * @file plugins.cpp
 * @brief  Brief description of file.
 *
 */

#include <dlfcn.h>
#include <strings.h>
#include <string.h>
#include <dirent.h>
#include <math.h>
#include <stdint.h>

#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <iostream>

#include "channel.h"
#include "exception.h"
#include "bounds.h"
#include "process.h"
#include "plugins.h"

using namespace std;

PluginData::PluginData(string l,const LADSPA_Descriptor *d){
    label=l;
    desc=d;
    // get the default port values
    for(unsigned int i=0;i<desc->PortCount;i++){
        const LADSPA_PortRangeHint *h = desc->PortRangeHints+i;
        int hd = h->HintDescriptor;
        float lower = h->LowerBound;
        float upper = h->UpperBound;
        
        if(h->HintDescriptor & LADSPA_HINT_SAMPLE_RATE){
            if(h->HintDescriptor & LADSPA_HINT_BOUNDED_BELOW)
                lower *= Process::samprate;
            if(h->HintDescriptor & LADSPA_HINT_BOUNDED_ABOVE)
                upper *= Process::samprate;
        }
        
        if(hd & LADSPA_HINT_DEFAULT_MINIMUM)
            defaultPortValues[i]=lower;
        else if(hd & LADSPA_HINT_DEFAULT_LOW){
            if(hd & LADSPA_HINT_LOGARITHMIC)
                defaultPortValues[i]=exp(log(lower)*0.75f+log(upper)*0.25f);
            else
                defaultPortValues[i]=lower*0.75f+upper*0.25f;
        } else if(hd & LADSPA_HINT_DEFAULT_MIDDLE){
            if(hd & LADSPA_HINT_LOGARITHMIC)
                defaultPortValues[i]=exp(log(lower)*0.5f+log(upper)*0.5f);
            else
                defaultPortValues[i]=lower*0.5f+upper*0.5f;
        } else if(hd & LADSPA_HINT_DEFAULT_HIGH){
            if(hd & LADSPA_HINT_LOGARITHMIC)
                defaultPortValues[i]=exp(log(lower)*0.25f+log(upper)*0.75f);
            else
                defaultPortValues[i]=lower*0.25f+upper*0.75f;
        } else if(hd & LADSPA_HINT_DEFAULT_MAXIMUM)
            defaultPortValues[i]=lower;
        else if(hd & LADSPA_HINT_DEFAULT_0)
            defaultPortValues[i]=0;
        else if(hd & LADSPA_HINT_DEFAULT_1)
            defaultPortValues[i]=0;
        else if(hd & LADSPA_HINT_DEFAULT_100)
            defaultPortValues[i]=100;
        else if(hd & LADSPA_HINT_DEFAULT_440)
            defaultPortValues[i]=440;
        // else there's no default and we write nothing. The
        // user must provide!
        
        if(defaultPortValues.find(i)!=defaultPortValues.end()){
            cout << "default for " << i << 
                  " (" << desc->PortNames[i] << ")" <<
                  " set to " << defaultPortValues[i] << endl;
        } else {
            cout << "Port " << i << 
                  " (" << desc->PortNames[i] << ")" <<
                  "has no default" << endl;
        }
    }
}

int PluginData::getPortIdx(string name){
    if(shortPortNames.find(name)!=shortPortNames.end()){
        return shortPortNames[name];
    } else {
        for(unsigned int i=0;i<desc->PortCount;i++){
            if(!strcmp(desc->PortNames[i],name.c_str())){
                return i;
            }
        }
    }
    throw _("cannot find port '%s' in plugin '%s'",
            name.c_str(),label.c_str());
}

bool PluginData::getDefault(string pname,float *f){
    int idx = getPortIdx(pname);
    if(defaultPortValues.find(idx)==defaultPortValues.end())
        return false;
    else
        *f = defaultPortValues[idx];
    return true;
}

Bounds PluginData::getBounds(string pname) {
    Bounds b;
    b.flags = 0;
    
    int idx = getPortIdx(pname);
    const LADSPA_PortRangeHint *h = desc->PortRangeHints+idx;
    if(h->HintDescriptor & LADSPA_HINT_BOUNDED_BELOW){
        b.lower = h->LowerBound;
        if(h->HintDescriptor & LADSPA_HINT_SAMPLE_RATE)
            b.lower *= Process::samprate;
        b.flags |= Bounds::Lower;
    }
    if(h->HintDescriptor & LADSPA_HINT_BOUNDED_ABOVE){
        b.upper = h->UpperBound;
        if(h->HintDescriptor & LADSPA_HINT_SAMPLE_RATE)
            b.upper *= Process::samprate;
        b.flags |= Bounds::Upper;
    }
    if(getDefault(pname,&b.deflt))
        b.flags |= Bounds::Default;
    if(h->HintDescriptor & LADSPA_HINT_LOGARITHMIC)
        b.flags |= Bounds::Log;
    return b;
}

void PluginData::addShortPortName(string shortname,string longname){
    for(unsigned int i=0;i<desc->PortCount;i++){
        if(!strcmp(desc->PortNames[i],longname.c_str())){
            shortPortNames[shortname]=i;
            return;
        }
    }
    throw _("cannot find port '%s' in plugin '%s'",
            longname.c_str(),label.c_str());
}




static std::vector<PluginInstance *> instances;

// instantiate this plugin and connect all control ports
// to default values - this may be overwritten by actual
// Values.
PluginInstance::PluginInstance(PluginData *plugin,string n) : portsConnected(128){
    p=plugin;
    name = n;
    h=(*p->desc->instantiate)(p->desc,Process::samprate);
    
    // connect up the ports to the defaults, and create output buffers
    for(unsigned int i=0;i<p->desc->PortCount;i++){
        portsConnected[i]=false;
        if(p->defaultPortValues.find(i)!=p->defaultPortValues.end()){
            float *addr = &(p->defaultPortValues[i]);
            cout << "Connecting port " << p->desc->PortNames[i]
                  << "(" << i << ") with " << addr <<endl;
            (*p->desc->connect_port)(h,i,addr);
            portsConnected[i]=true;
        }
        if(LADSPA_IS_PORT_OUTPUT(p->desc->PortDescriptors[i])){
            opbufs[i] = new float[BUFSIZE];
            cout << "Connecting OUTPUT port " << p->desc->PortNames[i]
                  << "(" << i << ") with " << opbufs[i] <<endl;
            (*p->desc->connect_port)(h,i,opbufs[i]);
        }
    }
    isActive=false;
    instances.push_back(this);
}

PluginInstance::~PluginInstance(){
    if(isActive)
        (*p->desc->deactivate)(h);
    (*p->desc->cleanup)(h);
    for(unsigned int i=0;i<p->desc->PortCount;i++){
        if(LADSPA_IS_PORT_OUTPUT(p->desc->PortDescriptors[i])){
            delete opbufs[i];
        }
    }
}

void PluginInstance::activate(){
    checkPortsConnected();
    (*p->desc->activate)(h);
    isActive=true;
}

void PluginInstance::connect(string name,float *v){
    int idx = p->getPortIdx(name);
    cout << "Connecting port " << name << " with address " << v << endl;
    (*p->desc->connect_port)(h,idx,v);
    portsConnected[idx]=true;
}

void PluginInstance::checkPortsConnected(){
    for(unsigned int i=0;i<p->desc->PortCount;i++){
        if(!portsConnected[i] && LADSPA_IS_PORT_CONTROL(p->desc->PortDescriptors[i]))
            throw _("port %d (%s) in plugin %s is not connected",
                    i,p->desc->PortNames[i],p->label.c_str());
    }
}
        
void PluginMgr::deleteInstances(){
    std::vector<PluginInstance *>::iterator it;
    for(it=instances.begin();it!=instances.end();it++){
        delete *it;
    }
}

PluginInstance *PluginData::instantiate(string name){
    return new PluginInstance(this,name);
}


/// these are the ladspa plugins we've already got
static unordered_map<string,PluginData *> plugins;
/// and whether we have that unique ID already.
static unordered_map<unsigned long,int> uniqueIDs;

void PluginMgr::loadFilesIn(const char *dir){
    DIR *d = opendir(dir);
    if(!dir)throw _("unable to open LADSPA directory %s",dir);
    
    while(dirent *e = readdir(d)){
        const char *s = rindex(e->d_name,'.');
        if(s){
            if(!strcmp(s+1,"so")){
                stringstream ss;
                ss << dir << "/" << e->d_name;
                const char *fname = ss.str().c_str();
                void *h = dlopen(fname,RTLD_NOW|RTLD_GLOBAL);
                if(h){
                    dlerror(); // clear error
                    LADSPA_Descriptor_Function getdesc = 
                          (LADSPA_Descriptor_Function)dlsym(h,"ladspa_descriptor");
                    if(!dlerror()){
                        // we just go through all the indices until
                        // we fail to get a plugin.
                        for(int i=0;;i++){
                            const LADSPA_Descriptor *desc = getdesc(i);
                            if(!desc)break;
                            string name = desc->Label;
                            cout << "Loaded plugin: " << name <<endl;
                            if(uniqueIDs.find(desc->UniqueID)==uniqueIDs.end()){
                                plugins.emplace(name,new PluginData(name,desc));
                                uniqueIDs.emplace(desc->UniqueID,1);
                            } else
                                cout << "ID CLASH" << endl;
                        }
                    }
//                    dlclose(h);
                }
            }
        }
    }
}

PluginData *PluginMgr::getPlugin(std::string label){
    if(plugins.find(label)==plugins.end())
        throw _("cannot find plugin '%s'",label.c_str());
    return plugins[label];
}

