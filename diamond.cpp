/**
 * @file diamond.cpp
 * @brief  Brief description of file.
 *
 */

#include <vector>
#include <unordered_map>
#if DIAMOND
#include <diamondapparatus/diamondapparatus.h>
#endif

#include "ctrl.h"
#include "exception.h"
#include "diamond.h"
#include "stringsplit.h"

using namespace std;

#if DIAMOND
using namespace diamondapparatus;
#endif

DiamondSource diamond;

// this gets hairy. Each topic can have a number of indices within it,
// and each index can have a number of ctrls.

// so each topic has a vector of controls indexed by index within the topic.
//
static unordered_map<string,unordered_map<int,vector<Ctrl*> > >sources;

void DiamondSource::init(){
#if DIAMOND
    try {
        diamondapparatus::init();
    } catch (DiamondException e){
        throw _("Fatal error in Diamond Apparatus: %s\n",e.what());
    }
#endif
}

const char *DiamondSource::add(string source,Ctrl *c){
#if DIAMOND
    try {
        vector<string> v = split(source,':');
        int index;
        if(v.size()==1)
            index = 0;
        else
            index = atoi(v[1].c_str());
    
        if(!sources.count(v[0])){
            // subscribe if the topic is new
            subscribe(v[0].c_str());
        }
        
        c->sourceInfo = new DiamondSourceInfo(v[0],index);
        c->source = this;
    
        sources[v[0]][index].push_back(c);
    } catch (DiamondException e){
        //        printf("Fatal error in Diamond Apparatus: %s\n",e.what());
        return e.what();
    }
#else
    throw "Diamond Apparatus not supported";
#endif
    return NULL;
}

void DiamondSource::remove(Ctrl *c){
#if DIAMOND
    DiamondSourceInfo *info = (DiamondSourceInfo*)(c->sourceInfo);
    vector<Ctrl *>& v = sources[info->topic][info->index];
    v.erase(std::remove(v.begin(),v.end(),c),v.end());
    delete info;
#endif
}

void DiamondSource::poll(){
    // iterate over all the topics, polling them. If we get new data,
    // go over the indices and throw data at the ctrls for each index.
    
#if DIAMOND
    unordered_map<string,unordered_map<int,vector<Ctrl*> > >
          ::iterator it;
    
    for(it=sources.begin();it!=sources.end();it++){
        const char *s = it->first.c_str();
        
        Topic t = get(s,GET_WAITNONE);
        if(t.isValid() && t.state==TOPIC_CHANGED){
            // iterate over the indices stored for this topic
            unordered_map<int,vector<Ctrl*> >::iterator it2;
            for(it2 = it->second.begin();it2!=it->second.end();it2++){
                float val;
                if(it2->first < t.size()) // make sure it's in the topic
                    val = t[it2->first].f(); // get value
                else
                    val = 0; // or use zero if not in topic
                
                // now iterate over the controls for this index of
                // this topic, and send it to the ringbuffer of that control
                vector<Ctrl*>::iterator it3;
                for(it3 = it2->second.begin();
                    it3!=it2->second.end();it3++){
                    (*it3)->setval(val);
                }
            }
        }
    }
#endif
}

