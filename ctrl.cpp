/**
 * @file ctrl.cpp
 * @brief  Brief description of file.
 *
 */
#include <sstream>
#include <iostream>

#include "ctrl.h"
#include "diamond.h"
#include "midi.h"
#include "save.h"

using namespace std;
unordered_map<string,Ctrl *> Ctrl::map;

Ctrl *Ctrl::createOrFind(string name,bool nocreate){
    Ctrl *v;
    unordered_map<string,Ctrl *>::iterator res;
    res = map.find(name);
    if(res==map.end()){
        if(nocreate)
            return NULL;
        v = new Ctrl(name);
        map.emplace(name,v);
    } else {
        v = res->second;
    }
    return v;
}

Ctrl::~Ctrl() {
    switch(sourceType){
    case DIAMOND:
        removeDiamondReferences(this);
        break;
    case MIDI:
        removeMidiReferences(this);
        break;
    default:break;
    }
    Value::removeCtrl(this);
    map.erase(nameString);
    delete ring;
}


vector<Ctrl *> Ctrl::getList(){
    vector<Ctrl *> lst;
    unordered_map<string,Ctrl *>::iterator it;
    for(it=map.begin();it!=map.end();it++){
        lst.push_back(it->second);
    }
    std::sort(lst.begin(),lst.end(),[](Ctrl *i,Ctrl *j)->bool{
              return i->nameString.compare(j->nameString)<0;});
    return lst;
}



void Ctrl::checkAllCtrlsForSource(){
    unordered_map<string,Ctrl *>::iterator it;
    for(it=map.begin();it!=map.end();it++){
        Ctrl *c = it->second;
        if(c->sourceType == NONE){
            string s = ("ctrl '"+it->first+ "' has no source defined");
            cout << s << endl;
        }
        /*
        vector<Value *>::iterator iv;
        for(iv=c->values.begin();iv!=c->values.end();iv++){
            Value *v = *iv;
            if(v->db != c->db){
                throw _("DB should match values and channels");
            }
           }
        */
    }
}

void Ctrl::removeAllAssociations(Value *v){
    unordered_map<string,Ctrl *>::iterator it;
    for(it=map.begin();it!=map.end();it++){
        Ctrl *c = it->second;
        c->remval(v);
    }
}

void Ctrl::pollAllCtrlRings(){
    unordered_map<string,Ctrl *>::iterator it;
    for(it=map.begin();it!=map.end();it++){
        Ctrl *c = it->second;
        c->pollRing();
    }
}


Ctrl *Ctrl::setsource(string spec){
    sourceString = spec;
    // just assume diamond for now
    addDiamondSource(spec,this);
    sourceType=DIAMOND;
    return this;
}

void Ctrl::save(ostream& out){
    out << "  " << nameString << ": \"" << sourceString << "\"";
    out << " in " << inmin << ":" << inmax;
}

void Ctrl::saveAll(ostream &out){
    out << "ctrl {\n";
    
    vector<string> strs;
    unordered_map<string,Ctrl *>::iterator it;
    for(it=map.begin();it!=map.end();it++){
        stringstream ss;
        Ctrl *c = it->second;
        // only bother with ctrls we actually have sources for 
        if(c->sourceType != NONE){ 
            c->save(ss);
            strs.push_back(ss.str());
        }
    }
    
    out << intercalate(strs,",\n");
    
    out << "\n}\n";
}
