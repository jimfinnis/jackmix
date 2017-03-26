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
#include "ctrl.h"

using namespace std;

vector<ChainInterface *> chainlist;
Value *parseValue(Bounds b,Value *v=NULL);
float *zeroBuf;

struct Chain : public ChainInterface {
    Chain() : ChainInterface() {
        // initially null, because there are no FX.
        leftoutbuf=zeroBuf;
        rightoutbuf=zeroBuf;
        leftouteffect="zero";
        rightouteffect="zero";
        leftoutport="zero";
        rightoutport="zero";
    }
    
    virtual ~Chain(){
        for(unsigned int i=0;i<fxlist.size();i++){
            PluginInstance *p = fxlist[i];
            delete p;
            vector<InputConnectionData> *ipdl = inputConnData[i];
            delete ipdl;
        }
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
    void resolveInputs(bool debugout=true){
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
                    if(debugout)cout << "Left input";
                    buf = inpleft;
                    break;
                case 1:
                    if(debugout)cout << "Right input";
                    buf = inpright;
                    break;
                case 2:
                    if(debugout)cout << "Zero";
                    buf = zeroBuf;
                    break;
                case -1:
                    if(debugout)cout << "Port " << ipd.fromeffect << ":" << ipd.fromport;
                    
                    buf = getPort(ipd.fromeffect,ipd.fromport);
                    break;
                default:
                    throw _("weird case in ipd channel: %d\n",ipd.channel);
                }
                
                // make the connection for the input port
                if(debugout){
                    cout << " has address " << buf;
                    cout << ", connecting to " << ipd.port << endl;
                }
                (*p->p->desc->connect_port)(p->h,ipd.port,buf);
                p->connections[ipd.port]=buf;
            }
        }
    }
    
    // resolve a possibly short port name to a proper one
    string getRealPortName(string effect,string port){
        if(effect=="zero")return "zero";
        if(fxmap.find(effect)==fxmap.end())
            throw _("cannot find effect '%s'",effect.c_str());
        PluginInstance *inst = fxmap[effect];
        int fpidx = inst->p->getPortIdx(port);
        return string(inst->p->desc->PortNames[fpidx]);
    }
    
    
    float *getPort(string effect,string port){
        if(effect=="zero")return zeroBuf;
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
//            printf("Running %s\n",p->name.c_str());
//            p->dump();
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
    
    // add a new effect  - from the processing thread!
    virtual void addEffect(PluginData *d,string name);
    
    void deleteEffect(int idx);
    
    virtual void remapInput(std::string instname,
                            std::string inpname,
                            int chan, // 0/1 for chain inputs, -1 for another effect
                            // below used when chan==-1
                            std::string outinstname,
                            std::string outname);
    virtual void remapOutput(int outchan,
                             std::string instname,
                             std::string port);
    
};


/// The effects chains are stored as an unordered map of string to chain.

static unordered_map<string,Chain> chains;

void ChainInterface::addNewEmptyChain(string n){
    Chain& chain = chains.emplace(n,Chain()).first->second;
    chain.name = n;
    chainlist.push_back(&chain);
    string retname = "R"+n;
    Value *g = new Value(n+" ret gain");
    g->setdb()->setdbrange()->setdef(-50)->reset();
    Value *p = new Value(n+" ret pan");
    p->setrange(0,1)->setdef(0.5)->reset();
    Channel *c = new Channel(retname,2,g,p,true,n);
    c->resolveReturnChannel();
}

void ChainInterface::deleteChain(int n){
    // assume chaininterfaces are all Chain
    Chain *chain = (Chain *)chainlist[n];
    
    // we need to remove any return channel and sends
    Channel::removeReturnChannelsAndSends(chain->name);
    
    // remove from the map
    chains.erase(chain->name);
    // this should delete/deactivate all the effects
    // by running the dtor
    chainlist.erase(chainlist.begin()+n);
}

void ChainInterface::deleteEffect(int chainidx,int fidx){
    // assume chaininterfaces are all Chain
    Chain *chain = (Chain *)chainlist[chainidx];
    
    chain->deleteEffect(fidx);
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
    PluginInstance *i = p->instantiate(name,c.name);
    
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
              else if(outname=="ZERO")ipd.channel = 2;
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
    
    parseList([&p,&i,&c,&name]{
              // get param name (long or short, string or ident)
              int t = tok.getnext();
              if(t!=T_STRING && t!=T_IDENT)
              expected("parameter name");
              string pname = tok.getstring();
              
              // parse the value, using the LADSPA hints for the param
              Value *v = parseValue(p->getBounds(pname));
              v->setname(c.name+"/"+name+"/"+pname);
              printf("Param %s: %f\n",pname.c_str(),v->get());
              // and connect it
              i->connect(pname,v->getAddr());
              
              // we're replacing a default value! Has this somehow got a ctrl attached to it?
              // that can only have happened if this is a duplicate entry.
              if(i->paramsMap[pname]->getCtrl())
                  throw _("duplicate entry in parameters? %s/%s:%s",
                      c.name.c_str(),name.c_str(),pname.c_str());
              
              
              
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
            case 2:
                ss << "ZERO"; break;
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

void Chain::addEffect(PluginData *d,string n){
    // here we have to add the effect to the chain and also 
    // revise the input connection structures
    
    // instantiate the plugin
    PluginInstance *inst = d->instantiate(n,name);
    
    // add a new input connection data block
    vector<InputConnectionData> *ipdp = new vector<InputConnectionData>();
    inputConnData.push_back(ipdp);
    
    // set up the input connection data block:
    for(unsigned int i=0;i<d->desc->PortCount;i++){
        // for each input, select the 0 or 1 channel (i.e. left or right inputs
        // into the chain, depending on whether the port name contains "right".
        if(LADSPA_IS_PORT_INPUT(d->desc->PortDescriptors[i])
           && LADSPA_IS_PORT_AUDIO(d->desc->PortDescriptors[i])){
            InputConnectionData ipd;
            
            ipd.channel=0; // normally left.. unless..
            const char *pn = d->desc->PortNames[i];
            if(strcasestr(pn,"right") || strcasestr(pn,"input r"))
                ipd.channel=1;
            
            ipd.port = i;
            ipdp->push_back(ipd);
            
            // and make the actual connection (this is the part done by resolveInputs()
            // when loading a file)
            
            float *buf = ipd.channel?inpleft:inpright;
            (*inst->p->desc->connect_port)(inst->h,ipd.port,buf);
            inst->connections[ipd.port]=buf;
        }
    }
    
    // activate the effect
    inst->activate();
    // add the effect to the chain
    fxlist.push_back(inst);
    fxmap[n]=inst;
}


void Chain::remapInput(std::string instname,
                       std::string inpname,
                       int chan, // 0/1 for chain inputs, -1 for another effect
                       // below used when chan==-1
                       std::string outinstname,
                       std::string outname){
    // here we go. Get the instance.
    if(fxmap.find(instname)==fxmap.end())
        return;
    
    // we're going to need both the index and the instance
    PluginInstance *inst = fxmap[instname];
    int instidx = -1;
    for(unsigned int i=0;i<fxlist.size();i++){
        if(fxlist[i]==inst){
            instidx = i;
            break;
        }
    }
    if(instidx<0)throw _("could not find instance in list");
    
    // now get the input we're remapping
    int portidx = inst->p->getPortIdx(inpname);
    
    // and the IPD for the connection for this input
    int inputidx=-1;
    vector<InputConnectionData> *ipdl = inputConnData[instidx];
    for(unsigned int i=0;i<ipdl->size();i++){
        InputConnectionData& ipd = (*ipdl)[i];
        if(ipd.port == portidx){
            inputidx = i;
        }
    }
    if(inputidx<0)throw _("could not find port");
    
    
    InputConnectionData& ipd = (*ipdl)[inputidx];
    
    if(chan>=0){
        // simple case - remap to chain input
        // do the remap in the IPD
        ipd.channel = chan;
        // and in the actual plugin
        float *buf;
        switch(chan){
        case 0:buf=inpleft;break;
        case 1:buf=inpright;break;
        case 2:buf=zeroBuf;break;
        }    
        (*inst->p->desc->connect_port)(inst->h,portidx,buf);
        inst->connections[portidx]=buf;
    } else {
        // otherwise we need to get the effect and port for the output we're
        // coming from
        
        if(fxmap.find(outinstname)==fxmap.end())
            throw _("cannot find output inst");
        
        // and find the output port
        PluginInstance *outinst = fxmap[outinstname];
        int outidx = outinst->p->getPortIdx(outname);
        float *buf=outinst->opbufs[outidx];
        
        // and link
        ipd.channel = -1;
        ipd.fromeffect = outinstname;
        ipd.fromport = outname;
        (*inst->p->desc->connect_port)(inst->h,portidx,buf);
        inst->connections[portidx]=buf;
    }
}

void Chain::remapOutput(int outchan,
                        std::string instname,
                        std::string port){
    // here we go. Get the instance.
    if(fxmap.find(instname)==fxmap.end())
        return;
    // we're going to need both the index and the instance
    PluginInstance *inst = fxmap[instname];
    int instidx = -1;
    for(unsigned int i=0;i<fxlist.size();i++){
        if(fxlist[i]==inst){
            instidx = i;
            break;
        }
    }
    if(instidx<0)throw _("could not find instance in list");
    
    // now get the port we'll be using as an output
    int idx = inst->p->getPortIdx(port);
    
    // check it's valid
    if(!LADSPA_IS_PORT_OUTPUT(inst->p->desc->PortDescriptors[idx])
       || !LADSPA_IS_PORT_AUDIO(inst->p->desc->PortDescriptors[idx])){
        throw _("not a valid port");
    }
    
    // and remap
    
    if(outchan){
        rightoutbuf = inst->opbufs[idx];
        rightouteffect = instname;
        rightoutport = port;
    } else {
        leftoutbuf = inst->opbufs[idx];
        leftouteffect = instname;
        leftoutport = port;
    }
    Channel::resolveAllChannelChains(); // make sure the return channels are right
        
}

void Chain::deleteEffect(int idx){
    PluginInstance *inst = fxlist[idx];
    
    // first, we need to remove this instance as an input for all instances that
    // feed into it, replacing them with the zero buffer.
    
    for(unsigned int i=0;i<fxlist.size();i++){
        // remove references from this effect into ipdl
        vector<InputConnectionData> *ipdl = inputConnData[i];
        vector<InputConnectionData>::iterator it;
        for(it=ipdl->begin();it!=ipdl->end();it++){
            InputConnectionData& d = (*it);
            if(d.channel == -1 && d.fromeffect == inst->name){
                d.fromeffect="zero";
                d.fromport="zero";
            }
        }
    }
    
    // remove this effect from the input connection list
    delete inputConnData[idx];
    inputConnData.erase(inputConnData.begin()+idx);
    
    // then we need to replace the output buffers with zero if they refer to this.
    
    if(leftouteffect == inst->name){
        leftouteffect = "zero";
        leftoutport = "zero";
        leftoutbuf = zeroBuf;
    }
    if(rightouteffect == inst->name){
        rightouteffect = "zero";
        rightoutport = "zero";
        rightoutbuf = zeroBuf;
    }
    
    // remove it from the map
    fxmap.erase(inst->name);
    // and from the list
    fxlist.erase(fxlist.begin()+idx);
    
    // now we can delete it.
    delete inst;
    
    
    // and fixup the inputs again
    resolveInputs(false);
}
