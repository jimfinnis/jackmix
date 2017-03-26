/**
 * @file screenctrl.cpp
 * @brief  
 * 
 *
 */

#include "monitor.h"
#include "process.h"
#include "proccmds.h"
#include "ctrl.h"

#include "screenmain.h"
#include "screenctrl.h"

#include <ncurses.h>
#include <sstream>

CtrlScreen scrCtrl;
static int curctrl=0;
static int curval=0;
static enum { CTRLLIST,VALLIST } mode=CTRLLIST;

void CtrlScreen::display(MonitorData *d){
    title("CONTROLLER EDIT");
    attrset(COLOR_PAIR(0));
    
    // on the lhs, show the controllers. On the RHS show the values
    // which this controller has
    
    vector<Ctrl *> lst = Ctrl::getList();
    int lsize = lst.size();
    if(curctrl<0)curctrl=lsize-1;
    if(curctrl>=lsize)curctrl=0;
    
    // pagination of ctrl and val lists
    int w,h;
    getmaxyx(stdscr,h,w);
    int ctrlpagesize = h-4;
    int valpagesize = h-16;
    
    int ctrlpage = curctrl<0 ? 0 : curctrl/ctrlpagesize;
    int valpage = curval<0 ? 0 : curval/valpagesize;
    int ctrlpagestart = ctrlpage * ctrlpagesize;
    int valpagestart = valpage * valpagesize;
    int ctrlpages = lsize/ctrlpagesize+1;
    
    for(int i=0;i<ctrlpagesize;i++){
        int idx = i+ctrlpagestart;
        if(idx<lsize){
            string n = lst[idx]->nameString;
            n.resize(20,' ');
            attrset((int)idx==curctrl ? COLOR_PAIR(PAIR_HILIGHT)
                    | (mode==CTRLLIST ? A_BOLD:0)
                          : COLOR_PAIR(0));
            mvaddstr(i+1,0,n.c_str());
        }
    }
    attrset(COLOR_PAIR(PAIR_BLUETEXT)|A_BOLD);
    mvprintw(ctrlpagesize+1,0,"page %d/%d",ctrlpage+1,ctrlpages);
    
    // show details
    if(curctrl>=0){
        Ctrl *c = lst[curctrl];
        
        attrset(0);
        mvaddstr(0,30,"Controller ");
        attrset(COLOR_PAIR(PAIR_HILIGHT));
        addstr(c->nameString.c_str());
        mvaddstr(1,30,"Values");
        attrset(0);
        int vlsize = c->values.size();
        int valpages = vlsize/valpagesize+1;
        if(curval<0)curval=vlsize-1;
        if(curval>=vlsize)curval=0;
        
        for(int i=0;i<valpagesize;i++){
            int idx = i+valpagestart;
            if(idx<vlsize){
                attrset((int)idx==curval ? COLOR_PAIR(PAIR_HILIGHT)
                        | (mode==VALLIST ? A_BOLD:0)
                              : COLOR_PAIR(0));
                stringstream ss;
                ss << c->values[idx]->name;
                mvaddstr(i+3,30,ss.str().c_str());
            }
        }
        attrset(COLOR_PAIR(PAIR_BLUETEXT)|A_BOLD);
        mvprintw(valpagesize+3,30,"page %d/%d",valpage+1,valpages);
        
    }
    
    attrset(0);
}

void CtrlScreen::flow(InputManager *im){
    vector<Ctrl *> lst = Ctrl::getList();
    int lsize = lst.size();
    if(curctrl<0)curctrl=lsize-1;
    if(curctrl>=lsize)curctrl=0;
    
    Ctrl *ctrl = curctrl>=0 ? lst[curctrl] : NULL;
    if(!ctrl)mode = CTRLLIST;
    int vlsize = ctrl ? ctrl->values.size() : 0;
    if(curval<0)curval=vlsize-1;
    if(curval>=vlsize)curval=0;
    
    
    int c = im->getKey();
    
    switch(c){
    case 9: // tab
        if(ctrl){
            mode = (mode==CTRLLIST)?VALLIST:CTRLLIST;
        }
        break;   
    case 'h':
        im->push();
        im->go(&scrHelp);
        break;
    case 'q':case 10:
        im->pop();
        break;
    case KEY_UP:
        if(mode == CTRLLIST)
            curctrl--;
        else
            curval--;
        break;
    case KEY_DOWN:
        if(mode == CTRLLIST)
            curctrl++;
        else 
            curval++;
        break;
    case KEY_DC:
        if(ctrl && im->getKey("Delete ctrl - are you sure?","yn")){
            ProcessCommand cmd(ProcessCommandType::DeleteCtrl);
            cmd.setctrl(ctrl);
            Process::writeCmd(cmd);
        }
        break;
    case'-':
        if(ctrl && curval>=0 && im->getKey("Delete value association - are you sure?","yn")){
            ProcessCommand cmd(ProcessCommandType::DeleteCtrlAssoc);
            cmd.setctrl(ctrl);
            cmd.setvalptr(ctrl->values[curval]);
            Process::writeCmd(cmd);
        }
        break;
        
    default:break;
    }
    
}
