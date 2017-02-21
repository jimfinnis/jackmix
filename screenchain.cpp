/**
 * @file screenchain.cpp
 * @brief  Brief description of file.
 *
 */
#include "monitor.h"
#include "process.h"
#include "proccmds.h"

#include "screenmain.h"
#include "screenchain.h"

#include <sstream>
#include <ncurses.h>
#include <unistd.h>

using namespace std;

ChainScreen scrChain;

// the chain we are currently viewing
static ChainEditData *chainData=NULL;
static volatile bool forceRegen=false;
// the effect within the chain we are currently viewing
static int cureffect=0;
// the parameter within the effect we are currently editing
static int cureffectparam=0;
// the chain we are currently viewing
static int curchain=0;

// what are we doing? Scrolling through chains, or looking at the effects?
enum ChainListMode{Chain,Effects,Params};
static ChainListMode chainListMode=Chain;

static void regenChainData(int idx){
    if(chainData)
        delete chainData;
    if(idx<(int)chainlist.size())
        chainData = chainlist[idx]->createEditData();
    else
        chainData = NULL;
    cureffect=0;
    cureffectparam=0;
}

void ChainScreen::display(MonitorData *d){
    if(forceRegen){
        regenChainData(curchain);
        forceRegen=false;
    }
    int w = MonitorThread::get()->w;
    title("CHAIN LIST");
    int chainHilight = COLOR_PAIR(PAIR_HILIGHT);
    int effectHilight = COLOR_PAIR(PAIR_HILIGHT);
    int paramHilight = COLOR_PAIR(PAIR_HILIGHT);
    
    switch(chainListMode){
    case Chain:chainHilight|=A_BOLD;break;
    case Effects:effectHilight|=A_BOLD;break;
    case Params:paramHilight|=A_BOLD;break;
    }
    
    MonitorThread::get()->lock();
    // list
    for(unsigned int i=0;i<chainlist.size();i++){
        string n = chainlist[i]->name.c_str();
        n.resize(20,' ');
        attrset((int)i==curchain ? chainHilight : COLOR_PAIR(0));
        mvaddstr(i+1,0,n.c_str());
    }
    //chain
    if(chainData){
        int x=25;
        int y=1;
        
        attrset(COLOR_PAIR(0));
        mvaddstr(y++,x,"left output from ");
        attrset(COLOR_PAIR(0)|A_BOLD);
        addstr(chainData->leftouteffect.c_str());
        addstr(":");
        addstr(chainData->leftoutport.c_str());
        attrset(COLOR_PAIR(0));
        mvaddstr(y++,x,"right output from ");
        attrset(COLOR_PAIR(0)|A_BOLD);
        addstr(chainData->rightouteffect.c_str());
        addstr(":");
        addstr(chainData->rightoutport.c_str());
        y++;
        attrset(COLOR_PAIR(0)|A_BOLD);
        mvaddstr(y++,x,"Effects:");
        // each effect
        for(unsigned int i=0;i<chainData->fx.size();i++){
            PluginInstance *fx= chainData->fx[i];
            vector<InputConnectionData> *icd = 
                  (*chainData->inputConnData)[i];
            
            // name of effect
            attrset(cureffect==(int)i?effectHilight:COLOR_PAIR(0));
            mvaddstr(y++,x,fx->name.c_str());
            
            // input connections
            for(unsigned int c=0;c<icd->size();c++){
                InputConnectionData &id = (*icd)[c];
                const char *portName = fx->p->desc->PortNames[id.port];
                attrset(COLOR_PAIR(0));
                mvaddstr(y++,x+20,portName);
                addstr(" from ");
                switch(id.channel){
                case 0:
                    attrset(COLOR_PAIR(PAIR_REDTEXT));
                    addstr("LEFT");break;
                case 1:
                    attrset(COLOR_PAIR(PAIR_REDTEXT));
                    addstr("RIGHT");break;
                default:
                    addstr(id.fromeffect.c_str());
                    addstr(":");
                    addstr(id.fromport.c_str());
                    break;
                }
            }
        }
        
        attrset(COLOR_PAIR(0));
        
        // current effect parameters
        y+=5;
        PluginInstance *fx;
        if(cureffect>=0 && cureffect<(int)chainData->fx.size())
            fx = chainData->fx[cureffect];
        else
            fx = NULL;
        if(fx){
            // might get weirded by a chain of chain or effect
            if(cureffectparam>=(int)fx->paramsList.size())
                cureffectparam=0;
            
            for(int i=0;i<(int)fx->paramsList.size();i++){
                string name = fx->paramsList[i];
                Value *v = fx->paramsMap[name];
                attrset(i==cureffectparam?paramHilight:COLOR_PAIR(0));
                mvaddstr(y,x,name.c_str());
                stringstream ss;
                ss << v->get() << "  range: "<< v->mn << " to " << v->mx;
                mvaddstr(y,x+50,ss.str().c_str());
                drawHorzBar(y+1,x,1,w-x-5,v->get(),v,Green,
                            i==cureffectparam && chainListMode==Params);
                
                y+=2;
            }
        }
    }
    MonitorThread::get()->unlock();
    attrset(COLOR_PAIR(0));
}

void ChainScreen::onEntry(){
    regenChainData(curchain);
}

void ChainScreen::addEffect(InputManager *im){
    bool ab;
    string ename = im->getFromList("Select effect (or CTRL-G to abort)",PluginMgr::pluginNames,&ab);
    if(ab||ename=="")return;
    
    try {
        PluginData *p = PluginMgr::getPlugin(ename);
        string iname = im->getString("Name",&ab);
        if(ab || iname=="")return;
        
        for(unsigned int i=0;i<chainData->fx.size();i++){
            if(chainData->fx[i]->name == iname){
                im->setStatus("effect instance of this name exists already",3);
                return;
            }
        }
        
        ProcessCommand cmd(ProcessCommandType::AddEffect);
        cmd.pld = p;
        cmd.arg0 = curchain;
        cmd.setstr(iname);
        Process::writeCmd(cmd);
        // have to wait for the chain to be added to regen data?
        //        im->getKey("Press key to regen");
        im->setStatus("REGENERATING",1);
        usleep(1000);
        forceRegen=true;
    }
    catch(string s){
        im->setStatus(s.c_str(),5);
    }
}

static void commandParamNudge(float v){
    if(chainData && cureffect>=0 && cureffect<(int)chainData->fx.size()){
        PluginInstance *fx = chainData->fx[cureffect];
        if(cureffectparam>=0 && cureffectparam<(int)fx->paramsList.size()){
            Value *p = fx->paramsMap[fx->paramsList[cureffectparam]];
            Process::writeCmd(ProcessCommand(ProcessCommandType::ChangeEffectParam,
                                             v,p));
        }
    }
}

void ChainScreen::remapInput(InputManager *im){
    PluginInstance *fx;
    bool ab;
    // initial command, needs filling in.
    ProcessCommand cmd(ProcessCommandType::RemapInput);
    
    if(cureffect>=0 && cureffect<(int)chainData->fx.size())
        fx = chainData->fx[cureffect];
    else {
        im->setStatus("no effect selected",3);return;
    }
    
    vector<InputConnectionData> *icd = 
          (*chainData->inputConnData)[cureffect];
    
    stringstream ss;
    ss << "Select input for " << fx->name << " to remap";
    
    // build list of inputs
    vector<string> strs;
    for(unsigned int c=0;c<icd->size();c++){
        InputConnectionData &id = (*icd)[c];
        const char *portName = fx->p->desc->PortNames[id.port];
        strs.push_back(portName);
    }
    string inputToChange = im->getFromList(ss.str(),strs,&ab);
    if(ab || inputToChange=="")return;
    
    // fill in that part of the effect
    cmd.setarg0(curchain)->
          setstr(fx->name)->
          setstr2(inputToChange);
    
    char cc = im->getKey("From left or right chain input, or effect output?","lre");
    
    if(cc=='l')
        cmd.arg1=0;
    else if(cc=='r')
        cmd.arg1=1;
    else {
        cmd.arg1=-1;
        
        // need to get an effect/output to come from
        // get a list of effects
        strs.clear();
        for(unsigned int i=0;i<chainData->fx.size();i++){
        strs.push_back(chainData->fx[i]->name);
        }
        ss.str("");
        ss << "select which effect " << inputToChange << " will come from";
        string effectForOutput = im->getFromList(ss.str(),strs,&ab);
        if(effectForOutput=="" || ab)return;
    
        // find that effect
        PluginInstance *outEffect=NULL;
        for(unsigned int i=0;i<chainData->fx.size();i++){
            if(chainData->fx[i]->name == effectForOutput){
                outEffect = chainData->fx[i];break;
            }
        }
        if(!outEffect){
            im->setStatus("weird, can't find that effect in my lists..",5);
            return;
        }
    
        // now get a list of that effect's outputs
        strs.clear();
        for(unsigned int i=0;i<outEffect->p->desc->PortCount;i++){
            if(LADSPA_IS_PORT_OUTPUT(outEffect->p->desc->PortDescriptors[i])
               && LADSPA_IS_PORT_AUDIO(outEffect->p->desc->PortDescriptors[i])){
                strs.push_back(outEffect->p->desc->PortNames[i]);
            }
        }
        ss.str("");
        ss << "Effect " << fx->name << ":" << inputToChange << " to come from " <<
              effectForOutput << ": ?";
        string portForOutput = im->getFromList(ss.str(),strs,&ab);
        if(portForOutput=="" || ab)return;
        
        cmd.setstr3(effectForOutput)->setstr4(portForOutput);
    }
    
    // and send
    Process::writeCmd(cmd);
    im->setStatus("REGENERATING",1);
    usleep(1000);
    forceRegen=true;
}

void ChainScreen::remapOutput(InputManager *im){
    PluginInstance *fx;
    bool ab;
    
    if(cureffect>=0 && cureffect<(int)chainData->fx.size())
        fx = chainData->fx[cureffect];
    else {
        im->setStatus("no effect selected",3);return;
    }
    
    // first pick an output
    
    char c = im->getKey("Remapping an output of this chain. Enter which one (or A to abort)","lra");
    if(c=='a')return;
    
    int chan = (c=='l')?0:1;
    
    vector<string> strs;
    for(unsigned int i=0;i<fx->p->desc->PortCount;i++){
        if(LADSPA_IS_PORT_OUTPUT(fx->p->desc->PortDescriptors[i])
           && LADSPA_IS_PORT_AUDIO(fx->p->desc->PortDescriptors[i])){
            strs.push_back(fx->p->desc->PortNames[i]);
        }
    }
    stringstream ss;
    ss << "Remapping " << (chan?"R":"L") << " chain output to an output of " 
          << fx->name << ". Which?";
    string port = im->getFromList(ss.str(),strs,&ab);
    if(ab || port=="")return;
    
    
    ProcessCommand cmd(ProcessCommandType::RemapOutput);
    cmd.setarg0(curchain)->setstr(fx->name)->setarg1(chan)->setstr2(port);
    
    Process::writeCmd(cmd);
    im->setStatus("REGENERATING",1);
    usleep(1000);
    forceRegen=true;
}

void ChainScreen::flow(InputManager *im){
    int c = im->getKey();
    PluginInstance *fx;
    bool ab;
    string name;
    
    switch(c){
    case 'q':
    case 10:
        im->pop(); // back to the previous screen
        break;
    case 'a':
        name = im->getString("Chain name",&ab);
        if(!ab && name!=""){
            if(ChainInterface::findornull(name)){
                im->setStatus("chain already exists",4);
            } else {
                im->lock();
                ChainInterface::addNewEmptyChain(name);
                im->unlock();
            }
        }
        break;
    case 'i':
        remapInput(im);
        break;
    case 'o':
        remapOutput(im);
        break;
    case 'e':
        addEffect(im);
        break;
    case 9: // tab
        switch(chainListMode){
        case Chain:chainListMode=Effects;break;
        case Effects:chainListMode=Params;break;
        case Params:chainListMode=Chain;break;
        }
        break;
    case 'h':
        im->push();
        im->go(&scrHelp);
        break;
    case KEY_UP:
        im->lock();
        switch(chainListMode){
        case Chain:curchain--;
            if(curchain<0)curchain=((int)chainlist.size())-1;
            regenChainData(curchain);
            break;
        case Effects:
            cureffect--;
            if(cureffect<0)cureffect=0;
            break;
        case Params:
            cureffectparam--;
            if(cureffectparam<0)cureffectparam=0;
            break;
        }
        im->unlock();
        break;
    case KEY_DOWN:
        im->lock();
        switch(chainListMode){
        case Chain:curchain++;
            if(curchain>=(int)chainlist.size())
                curchain=0;
            regenChainData(curchain);
            break;
        case Effects:
            cureffect++;
            if(chainData && cureffect>=(int)chainData->fx.size())
                cureffect=0;
            break;
        case Params:
            if(chainData && cureffect>=0 &&
               cureffect<(int)chainData->fx.size())
                fx = chainData->fx[cureffect];
            else
                fx = NULL;
            if(fx){
                cureffectparam++;
                if(cureffectparam>=(int)fx->paramsList.size())
                    cureffectparam=0;
            }
            break;
        }
        im->unlock();
        break;
    case KEY_LEFT:
        if(chainListMode==Params)
            commandParamNudge(-1);
        break;
    case KEY_RIGHT:
        if(chainListMode==Params)
            commandParamNudge(1);
        break;
    default:break;
    }
}
