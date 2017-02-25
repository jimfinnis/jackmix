/**
 * @file screenmain.cpp
 * @brief  Brief description of file.
 *
 */

#include "monitor.h"
#include "process.h"
#include "save.h"

#include "screenmain.h"
#include "screenchan.h"
#include "screenchain.h"

#include <ncurses.h>
#include <sstream>
MainScreen scrMain;

#define RIGHTWIDTH 20
#define COLWIDTH 10

static MonitorData lastDisplayed;


void MainScreen::display(MonitorData *d){
    title("MAIN VIEW");
    // how many channels (cols) can we do, not including MASTER?
    // There's a bit of space over
    // to the right we need to leave free.
    
    int w = MonitorThread::get()->w;
    
    int numcols = (w-RIGHTWIDTH)/COLWIDTH-1; // (-1 because master)
    
    // there is a current col, which must be on the screen.
    
    int firstcol = 0; // first col on screen
    
    // curchan may be -1, in which case we are editing master values.
    
    if(curchan >= firstcol+numcols){
        firstcol = curchan-numcols/2;
    }
    
    
    displayChan(0,&d->master,curchan==-1);
    curchanptr=NULL;
    for(int i=0;i<numcols;i++){
        ChanMonData *c;
        int chanidx = i+firstcol;
        if(chanidx>=0 && chanidx<(int)d->numchans){
            c=&d->chans[chanidx];
            if(chanidx==curchan)curchanptr=c->chan;
        } else
            c=NULL;
        if(c)
            displayChan(i+1,c,chanidx==curchan);
    }
}

void MainScreen::displayChan(int i,ChanMonData* c,bool cur){
    int x = i*COLWIDTH;
    
    float l,r,gain,pan;
    const char *name;
    
    int h = MonitorThread::get()->h;
    
    Value *pv,*gv;
    if(c){
        l=c->l;
        r=c->r;
        if(c->chan){
            pv=c->chan->pan;
            gv=c->chan->gain;
        } else {
            pv = Process::masterPan;
            gv = Process::masterGain;
        }
        
        gain=c->gain;
        pan=c->pan;
        name = c->name;
        
        if(c->chan){
            Channel *ch = c->chan;
            if(ch->isReturn()){
                attrset(COLOR_PAIR(0)|A_BOLD);
                string rn = ch->getReturnName();
                rn = "("+rn.substr(0,COLWIDTH-3)+")";
                mvaddstr(1,x,rn.c_str());
            }
            if(ch->isMute()){
                attrset(COLOR_PAIR(PAIR_BLUETEXT)|A_BOLD);
                mvaddstr(1,x,"MUTE");
            }
            if(ch->isSolo()){
                attrset(COLOR_PAIR(PAIR_REDTEXT)|A_BOLD);
                mvaddstr(1,x+5,"SOLO");
            }
        }
    } else {
        l=r=gain=pan=0;
        pv=gv=NULL;
        name="xxxx";
    }
    
    if(cur)
        attrset(COLOR_PAIR(PAIR_HILIGHT)|A_BOLD);
    else
        attrset(COLOR_PAIR(0));
    
    mvprintw(0,x,"%s",name);
    
    
    // inputs to pan/gain bars are range 0-1 unless a value is given
    drawVertBar(2,x,h-3,1,l,NULL,Gain,false);
    drawVertBar(2,x+2,h-3,1,r,NULL,Gain,false);
    drawVertBar(2,x+4,h-3,1,gain,gv,Green,cur);
    drawVertBar(2,x+6,h-3,1,pan,pv,Pan,cur);
    
    attrset(COLOR_PAIR(0));
}    

void MainScreen::commandGainNudge(float v){
    ProcessCommand cmd(ProcessCommandType::NudgeValue);
    if(curchanptr){
        if(curchanptr->gain->db)v*=0.1f;
        cmd.setvalptr(curchanptr->gain);
    } else {
        v*=0.1f; // master gain is always log
        cmd.setvalptr(Process::masterGain);
    }
    cmd.setfloat(v);
    Process::writeCmd(cmd);
}    

void MainScreen::commandPanNudge(float v){
    ProcessCommand cmd(ProcessCommandType::NudgeValue);
    cmd.setvalptr(curchanptr ? curchanptr->pan : Process::masterPan)->setfloat(v);
    Process::writeCmd(cmd);
}

void MainScreen::flow(InputManager *im){
    int c = im->getKey();
    switch(c){
    case 'p':
        if(curchanptr){
            stringstream ss;
            ss << curchanptr->name << " pan";
            im->editVal(ss.str(),curchanptr->pan);
        } else {
            im->editVal("Master pan",Process::masterPan);
        }
        break;
    case 'g':
        if(curchanptr){
            stringstream ss;
            ss << curchanptr->name << " gain";
            im->editVal(ss.str(),curchanptr->gain);
        } else {
            im->editVal("Master gain",Process::masterGain);
        }
        break;
    case 'h':
        im->push();
        im->go(&scrHelp);
        break;
    case 'a':{
        c=im->getKey("Stereo or mono","12sm");
        int chans = (c=='1' || c=='m') ? 1:2;
        bool ab,isret;
        string name = im->getString("New channel name",&ab);
        if(!ab && name.size()>0){
            if(Channel::getChannel(name,isret)){
                im->setStatus("Channel already exists",4);
            } else {
                ProcessCommand cmd(ProcessCommandType::AddChannel);
                cmd.setstr(name)->setarg0(chans);
                Process::writeCmd(cmd);
            }
        }
        break;
    }
    case 'r':{
        if(curchanptr){
            if(curchanptr->isReturn())
                im->setStatus("Channel is a return channel - delete the chain instead",4);
            else if(im->getKey("are you sure?","yn")=='y'){
                ProcessCommand cmd(ProcessCommandType::DelChan);
                cmd.setchan(curchanptr);
                Process::writeCmd(cmd);
            }
        } else
            im->setStatus("Cannot remove master channel",4);
        break;
    }
        
    case 'w':{
        bool ab;
        string name =im->getString("Filename",&ab);
        if(!ab){
            // we'd better lock, although nothing should be writing
            // this state.
            im->lock();
            saveConfig(name.c_str());
            im->unlock();
        }
        im->setStatus("Saved.",2);
    }
        break;
    case 'c':case 'C':
        im->push();
        im->go(&scrChain);
        break;
    case 'm':case 'M':
        if(curchanptr){
            ProcessCommand cmd(ProcessCommandType::ChannelMute);
            cmd.setchan(curchanptr);
            Process::writeCmd(cmd);
        }
        break;
    case 's':case 'S':
        if(curchanptr){
            ProcessCommand cmd(ProcessCommandType::ChannelSolo);
            cmd.setchan(curchanptr);
            Process::writeCmd(cmd);
        }
        break;
    case 10:
        if(curchan>=0){
            im->push();
            scrChan.setChan(curchan);
            im->go(&scrChan);
        }
        break;
    case 'x':
        curchan++;
        break;
    case 'z':
        if(--curchan<-1)
            curchan=0;
        break;
    case KEY_UP:
        commandGainNudge(1);break;
    case KEY_DOWN:
        commandGainNudge(-1);break;
    case KEY_LEFT:
        commandPanNudge(-1);break;
    case KEY_RIGHT:
        commandPanNudge(1);break;
    case 'q':case 'Q':
        c = im->getKey("are you sure?","yn");
        if(c=='y'||c=='Y')im->go(NULL);
    default:break;
    }
}
