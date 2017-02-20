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
#include "value.h"
#include "global.h"
#include "parser.h"
#include "fx.h"
#include "channel.h"
#include "save.h"

using namespace std;

vector<ChainInterface *> chainlist;
Value *parseValue(Bounds b,Value *v=NULL);

struct Chain : public ChainInterface {
    Chain() : ChainInterface() {
        // initially null, because there are no FX.
        leftoutbuf=NULL;
        rightoutbuf=NULL;
    }
    // vector so we can run in order
    vector<PluginInstance *> fxlist;
    // map so we can find
    unordered_map<string,PluginInstance *> fxmap;
    
    // names of things, kept for saving
    string rightoutport,leftoutport,rightouteffect,leftouteffect;
    
    // input connection data to resolve later, indexed
    // by same order as fxlist, then within that, by input.
    vector<vector<InputConnectionData>*> inputConnData;
    
    // resolve connections within the chain
    void resolveInputs(){
        if(fxlist.size()!=inputConnData.size())
            throw _("size mismatch in effect lists");
        
        // this wires up the input ports
        for(unsigned int i=0;i<fxlist.size();i++){
            PluginInstance *p = fxlist[i];
            vector<InputConnectionData> *ipdl = inputConnData[i];
            
            for(unsigned int j=0;j<ipdl->size();j++){
                InputConnectionData& ipd = (*ipdl)[j];
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
    
    // resolve a possibly short port name to a proper one
    string getRealPortName(string effect,string port){
        if(fxmap.find(effect)==fxmap.end())
            throw _("cannot find effect '%s'",effect.c_str());
        PluginInstance *inst = fxmap[effect];
        int fpidx = inst->p->getPortIdx(port);
        return string(inst->p->desc->PortNames[fpidx]);
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
    
    virtual ChainEditData *createEditData(){
        ChainEditData *d = new ChainEditData();
        vector<PluginInstance *>::iterator it;
        for(it = fxlist.begin();it!=fxlist.end();it++){
            PluginInstance *p = *it;
            d->fx.push_back(p);
        }
        d->inputConnData = &inputConnData;
        d->leftouteffect = leftouteffect;
        d->leftoutport = leftoutport;
        d->rightouteffect = rightouteffect;
        d->rightoutport = rightoutport;
        return d;
    }
    virtual void save(ostream &out,string name);
};


/// The effects chains are stored as an unordered map of string to chain.

static unordered_map<string,Chain> chains;

void ChainInterface::addNewEmptyChain(string n){
    Chain& chain = chains.emplace(n,Chain()).first->second;
    chain.name = n;
    chainlist.push_back(&chain);
    string retname = "R"+n;
    Value *g = new Value();
    g->setdb()->setrange(-60,0)->setdef(-50)->reset();
    Value *p = new Value();
    p->setrange(0,1)->setdef(0.5)->reset();
    Channel *c = new Channel(retname,2,g,p,true,n);
    c->resolveReturnChannel();
}


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
    PluginInstance *i = p->instantiate(name);
    
    // get the input names, we'll resolve them later
    
    if(tok.getnext()!=T_IN)expected("'in'");
    
    vector<InputConnectionData> *ipdp = new vector<InputConnectionData>();
    c.inputConnData.push_back(ipdp);
    
    parseList([&c,&p,&ipdp]{
              InputConnectionData ipd;
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
                ipd.fromport = getnextidentorstring();
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
              
        // parse the value, using the LADSPA hints for the param
        Value *v = parseValue(p->getBounds(pname));
        printf("Param %s: %f\n",pname.c_str(),v->get());
        // and connect it
        i->connect(pname,v->getAddr());
        delete i->paramsMap[pname];
        i->paramsMap[pname]=v;
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
    
    Chain& chain = chains.emplace(name,Chain()).first->second;
    chain.name = name;
    chainlist.push_back(&chain);
    
    
    // get the names of the output fx and ports (two, this is
    // a stereo chain
    if(tok.getnext()!=T_OUT)expected("'out'");
    chain.leftouteffect = getnextident();
    if(tok.getnext()!=T_COLON)expected("':'");
    chain.leftoutport = getnextidentorstring();
    
    if(tok.getnext()!=T_COMMA)expected("','");
    chain.rightouteffect = getnextident();
    if(tok.getnext()!=T_COLON)expected("':'");
    chain.rightoutport = getnextidentorstring();
    
    if(tok.getnext()!=T_FX)expected("'fx'");
    
    parseList([&chain]{
              parseEffect(chain);
          });
    if(tok.getnext()!=T_CCURLY)expected("'}'");
    
    // now all the effects are parsed and created, resolve
    // the internal references
    
    chain.resolveInputs();
    
    // and then get pointers to the output buffers
    
    chain.leftoutbuf = chain.getPort(
                                     chain.leftouteffect,
                                     chain.leftoutport);
    chain.rightoutbuf = chain.getPort(
                                      chain.rightouteffect,
                                      chain.rightoutport);
    
    // fixup the names - I know the way this is done is damn ugly, but
    // it's because of how the system was written :)
    chain.leftoutport = chain.getRealPortName(
                                     chain.leftouteffect,
                                     chain.leftoutport);
    chain.rightoutport = chain.getRealPortName(
                                      chain.rightouteffect,
                                      chain.rightoutport);
}
    
ChainInterface *ChainInterface::find(std::string name){
    if(chains.find(name)==chains.end())
        throw _("chain %s does not exist",name.c_str());
    
    return &chains[name];
}
ChainInterface *ChainInterface::findornull(std::string name){
    if(chains.find(name)==chains.end())
        return NULL;
    return &chains[name];
}

vector<string> ChainInterface::getNames(){
    vector<string> names;
    unordered_map<string,Chain>::iterator it;
    for(it=chains.begin();it!=chains.end();it++){
        names.push_back(it->first);
    }
    return names;
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

void Chain::save(ostream &out,string name){
    out << "  " << name << " {\n";
    
    out << "    " << "out " << leftouteffect << ": \"" << leftoutport << "\"";
    out << ", " << rightouteffect << ": \"" << rightoutport << "\"\n";
    
    out << "    fx {\n";
    
    vector<string> fxstrs;
    for(unsigned int pidx=0;pidx<fxlist.size();pidx++){
        stringstream fss;
        PluginInstance *p = fxlist[pidx];
        vector<InputConnectionData> *inp = inputConnData[pidx];
        
        fss << "      " << p->p->label << " " << p->name << "\n";
        fss << "      in {\n";
        vector<string> strs;
        for(unsigned int iidx=0;iidx<inp->size();iidx++){
            stringstream ss;
            InputConnectionData& ipd = (*inp)[iidx];
            ss << "        \"" << p->p->desc->PortNames[ipd.port] << "\"" ;
            ss << " from ";
            switch(ipd.channel){
            case 0:
                ss << "LEFT";break;
            case 1:
                ss << "RIGHT";break;
            case -1:
                ss << ipd.fromeffect << ":\"";
                ss << getRealPortName(ipd.fromeffect,ipd.fromport);
                ss << "\"";
                break;
            default:
                ss << "???"; break;
            }
            strs.push_back(ss.str());
        }
        fss << intercalate(strs,",\n");
        fss << "\n      }\n";
        
        fss << "      params {\n";
        strs.clear();
        unordered_map<string,Value *>::iterator it;
        for(it=p->paramsMap.begin();it!=p->paramsMap.end();it++){
            stringstream ss;
            ss << "        \"" << getRealPortName(p->name,it->first) << "\" ";
            ss << it->second->toString();
            strs.push_back(ss.str());
        }
        fss << intercalate(strs,",\n");
        fss << "\n      }";
        
        fxstrs.push_back(fss.str());
    }
    
    out << intercalate(fxstrs,",\n");
    out << "\n    }\n";
    
    out << "  }\n";
}

void ChainInterface::saveAll(ostream &out){
    out << "chain {\n";
    
    unordered_map<string,Chain>::iterator it;
    vector<string> strs;
    for(it=chains.begin();it!=chains.end();it++){
        stringstream ss;
        it->second.save(ss,it->first);
        strs.push_back(ss.str());
    }
    
    out << intercalate(strs,"\n,");
    
    out << "}\n";
}
