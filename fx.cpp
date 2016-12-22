/**
 * @file fx.cpp
 * @brief  Brief description of file.
 *
 */

#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>

#include "tokeniser.h"
#include "tokens.h"
#include "exception.h"
#include "plugins.h"
#include "value.h"

using namespace std;

extern Tokeniser tok;
extern string getnextident();
extern void expected(string);
Value *parseValue(bool deflt_ok=false,float defval=0);



/// bind a LADSPA parameter in a given instance of a plugin to
/// a Value.
//bindParameter(const LADSPA_Descriptor *desc, LADSPA_Handle instance,
//              string pname,Value *v){
//}



/// an effect, which contains the necessary data for LADSPA to run it,
/// having linked to Values for control.

struct FX {
    PluginData *p;
};

/// The effects chains are stored as an unordered map of string to chain.
/// Each chain is a vector of FX structures. Each effect has a link to
/// a LADSPA effect.

static unordered_map<string,vector<FX> > chains;



void parseEffectAndAdd(vector<FX>& chain){
    int t = tok.getnext();
    if(t!=T_STRING && t!=T_IDENT)
        expected("LADSPA effect name");
    string name = tok.getstring();
    cout << "Parsing effect " << name << endl;
    
    // get the plugin data here
    PluginData *p = PluginMgr::getPlugin(name);
    
    // Create an actual instance of the plugin
    PluginInstance *i = p->instantiate();
    
    // now read parameter values and connect them to ports.
    // Unconnected ports should be set to the default value by creating
    // dummy values - this is done by the plugin manager
    // This can be done explicitly with "default" for controllable
    // values (default takes the place of the number).
        
    if(tok.getnext()!=T_OCURLY)
        expected("'{'");
    for(;;){
        // get param name (long or short, string or ident)
        int t = tok.getnext();
        if(t!=T_STRING && t!=T_IDENT)
            expected("LADSPA effect name");
        string pname = tok.getstring();
        
        // get default value
        float def=0;
        bool hasdeflt = p->getDefault(pname,&def);
        
        // get value permitting "default" or not
        Value *v = parseValue(hasdeflt,def);
        
        // and connect it
        i->connect(pname,v->getAddr());
        
        t = tok.getnext();
        if(t==T_CCURLY)break;
        else if(t!=T_COMMA)expected("',' or '{'");
    }
    
    // ensure all ports are connected, and activate
    i->activate();
    
}

void parseChain(){
    string name = getnextident();
    cout << "Parsing chain " << name << endl;
    
    // add a new chain, error if already there
    if(chains.find(name)!=chains.end())
        throw _("chain %s already exists",name.c_str());
    
    vector<FX>& chain = chains.emplace(name,vector<FX>()).first->second;
    
    if(tok.getnext()!=T_OCURLY)
        expected("'{'");
    for(;;){
        parseEffectAndAdd(chain);
        int t = tok.getnext();
        if(t==T_CCURLY)break;
        else if(t!=T_COMMA)expected("',' or '{'");
    }
}
    

void parseFXChainList(){
    if(tok.getnext()!=T_OCURLY)
        expected("'{'");
    for(;;){
        parseChain();
        int t = tok.getnext();
        if(t==T_CCURLY)break;
        else if(t!=T_COMMA)expected("',' or '{'");
    }
}
