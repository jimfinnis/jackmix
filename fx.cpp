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
#include "global.h"
#include "parser.h"
#include "fx.h"

using namespace std;

Value *parseValue(bool deflt_ok=false,float defval=0);

// stores temporary data about an input connection for an effect
struct InputParseData {
    int port; // the port for this input
    
    // and where it comes from:
    int channel; // -1 if this is an internal connection, 0=left,1=right
    // if channel=-1, info about the effect this comes from
    string fromeffect,fromport;
};

struct Chain : public ChainInterface {
    // vector so we can run in order
    vector<PluginInstance *> fxlist;
    // map so we can find
    unordered_map<string,PluginInstance *> fxmap;
    
    
    // input connection data to resolve later, indexed
    // by same order as fxlist, then within that, by input.
    vector<vector<InputParseData>*> parseData;
    
    // resolve connections within the chain
    void resolveInputs(){
        if(fxlist.size()!=parseData.size())
            throw _("size mismatch in effect lists");
        
        // this wires up the input ports
        for(unsigned int i=0;i<fxlist.size();i++){
            PluginInstance *p = fxlist[i];
            vector<InputParseData> *ipdl = parseData[i];
            
            for(unsigned int j=0;j<ipdl->size();j++){
                InputParseData& ipd = (*ipdl)[j];
                float *buf;
                switch(ipd.channel){
                case 0:
                    cout << "Left input";
                    buf = inpleft;
                    break;
                case 1:
                    cout << "Right input";
                    buf = inpright;
                    break;
                case -1:
                    cout << "Port " << ipd.fromeffect << ":" << ipd.fromport;
                          
                    buf = getPort(ipd.fromeffect,ipd.fromport);
                    break;
                default:
                    throw _("weird case in ipd channel: %d\n",ipd.channel);
                }
                
                // make the connection for the input port
                cout << " has address " << buf;
                cout << ", connecting to " << ipd.port << endl;
                (*p->p->desc->connect_port)(p->h,ipd.port,buf);
            }
        }
    }
    
    float *getPort(string effect,string port){
        if(fxmap.find(effect)==fxmap.end())
            throw _("cannot find source effect '%s'",effect.c_str());
        PluginInstance *inst = fxmap[effect];
        int fpidx = inst->p->getPortIdx(port);
                    
        if(inst->opbufs.find(fpidx)==inst->opbufs.end())
            throw _("bad port as source port: %s:%s",
                    effect.c_str(),
                    port.c_str());
        return inst->opbufs[fpidx];
    }
    
    virtual void run(unsigned int nframes){
        vector<PluginInstance *>::iterator it;
        for(it = fxlist.begin();it!=fxlist.end();it++){
            PluginInstance *p = *it;
            (*p->p->desc->run)(p->h,nframes);
        }
    }
};


/// The effects chains are stored as an unordered map of string to chain.

static unordered_map<string,Chain> chains;


// parse an effect within a chain, and add to chain
void parseEffect(Chain &c){
    string label = getnextidentorstring();
    string name =  getnextident();
    
    if(c.fxmap.find(name)!=c.fxmap.end())
        throw _("effect %s already exists in chain",name.c_str());
    
    
    cout << "Parsing effect " << name << ": is a " << label << endl;
    
    // get the plugin data here (throws on failure to find)
    PluginData *p = PluginMgr::getPlugin(label);
    
    // Create an actual instance of the plugin
    PluginInstance *i = p->instantiate();
    
    // get the input names, we'll resolve them later
    
    if(tok.getnext()!=T_IN)expected("'in'");
    
    vector<InputParseData> *ipdp = new vector<InputParseData>();
    c.parseData.push_back(ipdp);
    
    parseList([&c,&p,&ipdp]{
              InputParseData ipd;
              string pname = getnextidentorstring();
              ipd.port = p->getPortIdx(pname);
              if(!LADSPA_IS_PORT_INPUT(p->desc->PortDescriptors[ipd.port]))
                  throw _("%s is not an input port",pname.c_str());
                
              if(tok.getnext()!=T_FROM)expected("'from'");
              // this is either LEFT, RIGHT or an effect name, in which
              // case it is followed by ":port". These have to be resolved
              // later.
              string outname = getnextident();
              if(outname=="LEFT")ipd.channel = 0;
              else if(outname=="RIGHT")ipd.channel = 1;
              else {
                ipd.fromeffect = outname;
                if(tok.getnext()!=T_COLON)expected("':'");
                ipd.fromport = getnextident();
                ipd.channel = -1;
              }
              ipdp->push_back(ipd);
          }
     );
        
    
    
    // now read parameter values and connect them to ports.
    // Unconnected ports should be set to the default value by creating
    // dummy values - this is done by the plugin manager
    // This can be done explicitly with "default" for controllable
    // values (default takes the place of the number).
    
    
        
    if(tok.getnext()!=T_PARAMS)expected("'params'");
    
    parseList([&p,&i]{
        // get param name (long or short, string or ident)
        int t = tok.getnext();
        if(t!=T_STRING && t!=T_IDENT)
            expected("parameter name");
        string pname = tok.getstring();
        
        // get default value
        float def=0;
        bool hasdeflt = p->getDefault(pname,&def);
        
        // get value permitting "default" or not
        Value *v = parseValue(hasdeflt,def);
              printf("Param %s: %f\n",pname.c_str(),v->get());
        // and connect it
        i->connect(pname,v->getAddr());
    });
    
    // ensure all ports are connected, and activate
    i->activate();
    // add to chain data
    c.fxlist.push_back(i);
    c.fxmap[name]=i;
}

void parseStereoChain(){
    string name = getnextident();
    cout << "Parsing chain " << name << endl;
    
    // add a new chain, error if already there
    if(chains.find(name)!=chains.end())
        throw _("chain %s already exists",name.c_str());
    
    if(tok.getnext()!=T_OCURLY)expected("'{'");
    
    // get the names of the output fx and ports (two, this is
    // a stereo chain
    if(tok.getnext()!=T_OUT)expected("'out'");
    string leftouteffect = getnextident();
    if(tok.getnext()!=T_COLON)expected("':'");
    string leftoutport = getnextident();
    
    if(tok.getnext()!=T_COMMA)expected("','");
    string rightouteffect = getnextident();
    if(tok.getnext()!=T_COLON)expected("':'");
    string rightoutport = getnextident();
    
    Chain& chain = chains.emplace(name,Chain()).first->second;
    
    if(tok.getnext()!=T_FX)expected("'fx'");
    
    parseList([&chain]{
              parseEffect(chain);
          });
    if(tok.getnext()!=T_CCURLY)expected("'}'");
    
    // now all the effects are parsed and created, resolve
    // the internal references
    
    chain.resolveInputs();
    
    // and then get pointers to the output buffers
    
    chain.leftoutbuf = chain.getPort(leftouteffect,leftoutport);
    chain.rightoutbuf = chain.getPort(rightouteffect,rightoutport);
}
    
ChainInterface *ChainInterface::find(std::string name){
    if(chains.find(name)==chains.end())
        throw _("chain %s does not exist",name.c_str());
    
    return &chains[name];
}

void ChainInterface::zeroAllInputs(){
    unordered_map<string,Chain>::iterator it;
    for(it=chains.begin();it!=chains.end();it++){
        it->second.zeroInputs();
    }
}

void ChainInterface::runAll(unsigned int nframes){
    unordered_map<string,Chain>::iterator it;
    for(it=chains.begin();it!=chains.end();it++){
        it->second.run(nframes);
    }
}

