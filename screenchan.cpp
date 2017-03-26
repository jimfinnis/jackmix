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
#include <sstream>

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
        drawHorzBar(3,10,1,ww,c->l,NULL,VU,false);
        drawHorzBar(5,10,1,ww,c->r,NULL,VU,false);
        drawHorzBar(8,10,1,ww,c->gain,c->chan->gain,Gain,curparam==0);
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
    
    
    Value *curSelectedValue=NULL;
    if(chanidx>=0 && chanidx<d->numchans){
        switch(curparam){
        case 0:curSelectedValue = chan->gain;break;
        case 1:curSelectedValue = chan->pan;break;
        default:
            if(cursend>=0){
                ChainFeed& f = chan->chains[cursend];
                curSelectedValue = f.gain;
            }
            break;
        }
    }
    
    
    int c = im->getKey();
    
    switch(c){
    case 'h':
        im->push();
        im->go(&scrHelp);
        break;
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
                    ProcessCommand cmd(ProcessCommandType::AddSend);
                    cmd.setchan(chan)->setstr(sel);
                    Process::writeCmd(cmd);
                }
            }
        }
        break;
    case KEY_DC: // delete send
        if(cursend>=0 && chan){
            if(im->getKey("Delete send - are you sure?","yn")=='y'){
                ProcessCommand cmd(ProcessCommandType::DelSend);
                cmd.setarg0(cursend)->setchan(chan);
                Process::writeCmd(cmd);
            }
        }
    case 't':
        if(chan && cursend>=0){
            ProcessCommand cmd(ProcessCommandType::TogglePrePost);
            cmd.setarg0(cursend)->setchan(chan);
            Process::writeCmd(cmd);
        }
        break;
    case 'm':
        if(chan){
            ProcessCommand cmd(ProcessCommandType::ChannelMute);
            cmd.setchan(chan);
            Process::writeCmd(cmd);
        }
        break;
    case 's':
        if(chan){
            ProcessCommand cmd(ProcessCommandType::ChannelSolo);
            cmd.setchan(chan);
            Process::writeCmd(cmd);
        }
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
    } break;
    case 'c':
        if(curSelectedValue)
            commandAddCtrl(curSelectedValue);
        break;
    case 'r':
        if(curSelectedValue && curSelectedValue->getCtrl()){
            if(im->getKey("Delete controller association - are you sure?","yn")=='y'){
                ProcessCommand cmd(ProcessCommandType::DeleteCtrlAssoc);
                cmd.setctrl(curSelectedValue->getCtrl())->setvalptr(curSelectedValue);
                Process::writeCmd(cmd);
            }
        }
        break;
    case 'g':
        im->editVal("gain",chan->gain);break;
    case 'p':
        im->editVal("pan",chan->pan);break;
    case 'v':
        switch(curparam){
        case 0:im->editVal("gain",chan->gain);break;
        case 1:im->editVal("pan",chan->pan);break;
        default:
            if(cursend>=0){
                ChainFeed& f = chan->chains[cursend];
                stringstream ss;
                ss << (f.chain->name) << " send gain";
                im->editVal(ss.str(),f.gain);
            }
            break;
        }
        break;
    case 'q':case 10:
        im->pop();
        break;
    default:break;
    }
}

void ChanScreen::commandSendGainNudge(struct Channel *c,int send,float v){
    // send must be valid
    ChainFeed& f = c->chains[send];
    if(f.gain->db)v*=0.1f;
    ProcessCommand cmd(ProcessCommandType::NudgeValue);
    cmd.setvalptr(f.gain)->setfloat(v);
    Process::writeCmd(cmd);
                                     
}

void ChanScreen::commandGainNudge(struct Channel *c,float v){
    if(c->gain->db)v*=0.1f;
    ProcessCommand cmd(ProcessCommandType::NudgeValue);
    cmd.setvalptr(c->gain)->setfloat(v);
    Process::writeCmd(cmd);
}    

void ChanScreen::commandPanNudge(struct Channel *c,float v){
    ProcessCommand cmd(ProcessCommandType::NudgeValue);
    cmd.setvalptr(c->pan)->setfloat(v);
    Process::writeCmd(cmd);
}
    
