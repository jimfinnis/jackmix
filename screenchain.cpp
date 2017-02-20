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

using namespace std;

ChainScreen scrChain;

// the chain we are currently viewing
static ChainEditData *chainData=NULL;
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
        addstr(chainData->leftouteffect.c_str());
        addstr(":");
        addstr(chainData->leftoutport.c_str());
        mvaddstr(y++,x,"right output from ");
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
    attrset(COLOR_PAIR(0));
}

void ChainScreen::onEntry(){
    regenChainData(curchain);
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

void ChainScreen::flow(InputManager *im){
    int c = im->getKey();
    PluginInstance *fx;
    bool ab;
    string name;
    
    switch(c){
    case 10:
        im->pop(); // back to the previous screen
        break;
    case 'a':
        name = im->getString("Chain name",&ab);
        if(!ab){
            if(ChainInterface::findornull(name)){
                im->setStatus("chain already exists",4);
            } else {
                im->lock();
                ChainInterface::addNewEmptyChain(name);
                im->unlock();
            }
        }
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
