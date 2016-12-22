/**
 * @file ctrl.cpp
 * @brief  Brief description of file.
 *
 */
#include "ctrl.h"
#include "diamond.h"

#include <iostream>
std::unordered_map<std::string,Ctrl *> Ctrl::map;

Ctrl *Ctrl::createOrFind(std::string name){
    Ctrl *v;
    std::unordered_map<std::string,Ctrl *>::iterator res;
    res = map.find(name);
    if(res==map.end()){
        v = new Ctrl();
        map.emplace(name,v);
    } else {
        v = res->second;
    }
    return v;
}

void Ctrl::checkAllCtrlsForSource(){
    std::unordered_map<std::string,Ctrl *>::iterator it;
    for(it=map.begin();it!=map.end();it++){
        Ctrl *c = it->second;
        if(!c->hasSource){
            std::string s = ("ctrl '"+it->first+ "' has no source defined");
            std::cout << s << std::endl;
        }
        /*
        std::vector<Value *>::iterator iv;
        for(iv=c->values.begin();iv!=c->values.end();iv++){
            Value *v = *iv;
            if(v->db != c->db){
                throw "DB should match values and channels";
            }
           }
        */
    }
}

void Ctrl::pollAllCtrlRings(){
    std::unordered_map<std::string,Ctrl *>::iterator it;
    for(it=map.begin();it!=map.end();it++){
        Ctrl *c = it->second;
        c->pollRing();
    }
}


Ctrl *Ctrl::setsource(std::string spec){
    // just assume diamond for now
    addDiamondSource(spec,this);
    
    hasSource=true;
    return this;
}
