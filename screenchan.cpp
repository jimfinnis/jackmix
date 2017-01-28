/**
 * @file screenchan.cpp
 * @brief  Brief description of file.
 *
 */

#include "monitor.h"
#include "process.h"
#include "proccmds.h"

#include "screenmain.h"
#include "screenchan.h"

#include <ncurses.h>

ChanScreen scrChan;

void ChanScreen::display(MonitorData *d){
    title("CHANNEL EDIT");
    attrset(COLOR_PAIR(PAIR_HILIGHT)|A_BOLD);
    
    if(chanidx<0 || chanidx>=d->numchans){
        mvprintw(0,0,"Invalid channel");
    } else {
        int w = MonitorThread::get()->w;
        
        ChanMonData *c = d->chans+chanidx;
        Channel *curchanptr = c->chan;
        string namestr = c->name;
        
        if(curchanptr->isReturn()){
            string rn = curchanptr->getReturnName();
            namestr = namestr+" (return from chain "+rn+")";
        }
        mvprintw(0,0,"Channel %s",namestr.c_str());
        
        int numsends = (int)curchanptr->chains.size();
        // it's ugly checking this here, but it's a safeish
        // and convenient place
        if(curparam-2 >= numsends)
            curparam = 1+numsends;
        
        int cursend = curparam-2; // currently edited send
        
        
        if(curchanptr->isMute()){
            attrset(COLOR_PAIR(PAIR_BLUETEXT)|A_BOLD);
            mvaddstr(0,20,"MUTE");
        }
        if(curchanptr->isSolo()){
            attrset(COLOR_PAIR(PAIR_REDTEXT)|A_BOLD);
            mvaddstr(0,25,"SOLO");
        }
        
        attrset(COLOR_PAIR(0));
        mvaddstr(3,0,"Left");
        mvaddstr(5,0,"Right");
        mvaddstr(8,0,"Gain");
        mvaddstr(10,0,"Pan");
        
        int ww = w-20;
        drawHorzBar(3,10,1,ww,c->l,NULL,Gain,false);
        drawHorzBar(5,10,1,ww,c->r,NULL,Gain,false);
        drawHorzBar(8,10,1,ww,c->gain,c->chan->gain,Green,curparam==0);
        drawHorzBar(10,10,1,ww,c->pan,c->chan->pan,Pan,curparam==1);
        
        for(unsigned int i=0;i<curchanptr->chains.size();i++){
            int y = i*3+15;
            int ww = w-20;
            ChainFeed& f = curchanptr->chains[i];
            attrset(COLOR_PAIR(0));
            mvaddstr(y,0,curchanptr->chainNames[i].c_str());
            
            mvaddstr(y,20,f.postfade?"POSTFADE":"PREFADE");
            
            drawHorzBar(y+1,10,1,ww,f.gain->get(),f.gain,Green,
                        cursend==(int)i);
        }
    }
    
    attrset(COLOR_PAIR(0));
}

void ChanScreen::flow(InputManager *im){
    bool aborted;
    im->lock();
    MonitorData *d = MonitorThread::get()->getLastReceived();
    bool validchan = (chanidx>=0 && chanidx<d->numchans);
    ChanMonData *mon = validchan ? d->chans+chanidx : NULL;
    Channel *chan = mon ? mon->chan : NULL;
    int cursend = curparam-2;
    if(cursend>(int)chan->chains.size())cursend=-1; // flag invalid send
    im->unlock();
    
    int c = im->getKey();
    
    switch(c){
    case 'a':
        if(!validchan)
            im->setStatus("no valid channel selected",5);
        else {
            vector<string> names = ChainInterface::getNames();
            if(names.size()==0)
                im->setStatus("no chains to send to",5);
            else {
                string sel = im->getFromList("Select chain to send to (or CTRL-G to abort)",
                                             names,&aborted);
                if(!aborted){
                    Process::writeCmd(ProcessCommand(ProcessCommandType::AddSend,
                                                     chan,sel));
                }
            }
        }
        break;
    case KEY_DC: // delete send
        if(cursend>=0 && chan){
            if(im->getKey("Delete send - are you sure?","yn")=='y'){
            Process::writeCmd(ProcessCommand(
                                             ProcessCommandType::DelSend,
                                             0,chan,cursend));
                
            }
        }
    case 'p':
        if(chan && cursend>=0){
            Process::writeCmd(ProcessCommand(
                                             ProcessCommandType::TogglePrePost,
                                             0,chan,cursend));
        }
        break;
    case 'm':
        Process::writeCmd(ProcessCommand(ProcessCommandType::ChannelMute,chan));
        break;
    case 's':
        Process::writeCmd(ProcessCommand(ProcessCommandType::ChannelSolo,chan));
        break;
    case KEY_UP:
        if(--curparam < 0)curparam=0;
        break;
    case KEY_DOWN:
        curparam++; // out-of-range dealt with by display
        break;
    case KEY_LEFT:
    case KEY_RIGHT:{
        float v = c==KEY_LEFT ? -1 : 1;
        if(validchan)switch(curparam){
        case 0:commandGainNudge(chan,v);break;
        case 1:commandPanNudge(chan,v);break;
        default:if(cursend>=0)
                  commandSendGainNudge(chan,cursend,v);break;
        }
    }
        break;
    case 'q':case 10:
        im->go(&scrMain);
        break;
    default:break;
    }
}

void ChanScreen::commandSendGainNudge(struct Channel *c,int send,float v){
    // send must be valid
    ChainFeed& f = c->chains[send];
    if(f.gain->db)v*=0.1f;
    Process::writeCmd(ProcessCommand(ChangeSendGain,v,c,send));
                                     
}

void ChanScreen::commandGainNudge(struct Channel *c,float v){
    if(c->gain->db)v*=0.1f;
    Process::writeCmd(ProcessCommand(ProcessCommandType::ChangeGain,c,v));
}    

void ChanScreen::commandPanNudge(struct Channel *c,float v){
    Process::writeCmd(ProcessCommand(ProcessCommandType::ChangePan,c,v));
}
