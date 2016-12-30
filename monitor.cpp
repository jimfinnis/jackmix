/**
 * @file monitor.cpp
 * @brief  Brief description of file.
 *
 */

#include <ncurses.h>

#include "monitor.h"
#include "ringbuffer.h"
#include "exception.h"

#define PAIR_RED 1
#define PAIR_YELLOW 2
#define PAIR_GREEN 3
#define PAIR_HILIGHT 4

MonitorUI::MonitorUI(){
    initscr();
    keypad(stdscr,TRUE);
    noecho();
    cbreak();
    nl();
    curs_set(0);
    
    timeout(0); // nonblocking read
    
    start_color();
    init_pair(PAIR_HILIGHT,COLOR_YELLOW,COLOR_BLACK);
    init_pair(PAIR_GREEN,COLOR_GREEN,COLOR_GREEN);
    init_pair(PAIR_YELLOW,COLOR_YELLOW,COLOR_YELLOW);
    init_pair(PAIR_RED,COLOR_RED,COLOR_RED);
    
}

MonitorUI::~MonitorUI(){
    printf("Destructor\n");
    endwin();
}

static MonitorData *gentestdat(){
    static MonitorData *m=NULL;
    if(!m){
        m=new MonitorData;
//        m->chans.push_back(ChanMonData("foo",1,1,1,1));
//        m->chans.push_back(ChanMonData("bar",0.5,1,1,0.5));
//        m->chans.push_back(ChanMonData("baz",1,0.5,0.75,1));
    }
    return m;
}

void MonitorUIBasic::display(MonitorData *d){
    printf("MASTER: [%2.3f,%2.3f] gain %2.3f pan %2.3f\n",
           d->master.l,d->master.r,d->master.gain,d->master.pan);
    
    
    for(int i=0;i<d->numchans;i++){
        ChanMonData *c = d->chans+i;
        printf("  %s [%2.3f,%2.3f] gain %2.3f pan %2.3f\n",
               c->name,
               c->l,c->r,c->gain,c->pan);
    }
}


static MonitorData lastDisplayed;

void MonitorUI::display(MonitorData *d){
    getmaxyx(stdscr,h,w);
    if(!d)d=gentestdat();
    erase();
    displayChans(d);
    
    lastDisplayed=*d;
    
    refresh();
}

#define RIGHTWIDTH 20
#define COLWIDTH 10

void MonitorUI::displayChans(MonitorData *d){
    
    // how many channels (cols) can we do, not including MASTER?
    // There's a bit of space over
    // to the right we need to leave free.
    
    int numcols = (w-RIGHTWIDTH)/COLWIDTH-1; // (-1 because master)
    
    // there is a current col, which must be on the screen.
    
    int firstcol = 0; // first col on scren
    
    if(curchan >= firstcol+numcols){
        firstcol = curchan-numcols/2;
    }
    
    displayChan(0,&d->master,false);
    
    for(int i=0;i<numcols;i++){
        ChanMonData *c;
        int chanidx = i+firstcol;
        if(chanidx>=0 && chanidx<(int)d->numchans)
            c=&d->chans[chanidx];
        else
            c=NULL;
        displayChan(i+1,c,chanidx==curchan);
    }
}

void MonitorUI::displayChan(int i,ChanMonData* c,bool cur){
    int w,h;
    getmaxyx(stdscr,h,w);
    int x = i*COLWIDTH;
    
    if(cur)
        attrset(COLOR_PAIR(PAIR_HILIGHT)|A_BOLD);
    else
        attrset(COLOR_PAIR(0));
    
    float l,r,gain,pan;
    const char *name;
    if(c){
        l=c->l;
        r=c->r;
        gain=c->gain;
        pan=c->pan;
        name = c->name;
    } else {
        l=r=gain=pan=0;
        name="xxxx";
    }
    mvprintw(0,x,"%s",name);
    drawVertBar(2,x,h-3,1,l,Gain,false);
    drawVertBar(2,x+2,h-3,1,r,Gain,false);
    drawVertBar(2,x+4,h-3,1,gain,Green,cur && mode==EditGain);
    drawVertBar(2,x+6,h-3,1,pan,Pan,cur && mode==EditPan);
    
    if(cur){
        const char *modestr;
        switch(mode){
        case EditGain:modestr="GAIN (NYI)";break;
        case EditPan:modestr="PAN (NYI)";break;
        default:modestr="???";
        }
        attrset(COLOR_PAIR(0)|A_BOLD);
        mvaddstr(h-1,x,modestr);
    }
}


void MonitorUI::drawVertBar(int y, int x, int h, int w, 
                            float v,BarMode mode,bool bold){
    
    int val = v*h;
    int yellow = h*0.7f;
    int red = h*0.9f;
    int half = h/2;
    
    for(int i=0;i<h;i++){
        int col;
        bool rev=true;
        switch(mode){
        case Gain:
            if(i>val){
                col=0;rev=false;}
            else if(i>red)
                col=PAIR_RED;
            else if(i>yellow)
                col=PAIR_YELLOW;
            else 
                col=PAIR_GREEN;
            break;
        case Pan:
            if(val<half){
                if(i>half || i<val){col=0;rev=false;}
                else col=PAIR_GREEN;
            } else {
                if(i<half || i>val){col=0;rev=false;}
                else col=PAIR_GREEN;
            }
            break;
        case Green:
            if(i>val){col=0;rev=false;}
            else col = PAIR_GREEN;
        }
        long attr = bold ? A_BOLD: 0;
        if(rev)attr|=A_REVERSE;
        attrset(COLOR_PAIR(col) | attr);
        mvaddch(h-(y+i)+3,x,' ');
        for(int j=1;j<w;j++)addch(' ');
    }
}

extern RingBuffer<MonitorCommand> moncmdring;

void MonitorUI::command(MonitorCommandType cmd,float v,Channel *c){
    if(moncmdring.canWrite()){
        moncmdring.write(MonitorCommand(cmd,v,c));
    }
}

void MonitorUI::commandUpDown(float v){
    Channel *c = lastDisplayed.chans[curchan].chan;
    switch(mode){
    case EditGain:
        if(c->gain->db)v*=0.1f;
        command(MonitorCommandType::ChangeGain,v,c);
        break;
    case EditPan:
        if(c->gain->db)v*=0.1f;
        command(MonitorCommandType::ChangePan,v,c);
        break;
    default:
        break;
    }
}    

void MonitorUI::handleInput(){
    switch(getch()){
    case KEY_RIGHT:
        curchan++;
        break;
    case KEY_LEFT:
        if(--curchan<0)
            curchan=0;
        break;
    case KEY_UP:
        commandUpDown(1);break;
    case KEY_DOWN:
        commandUpDown(-1);break;
    case 9: // tab
        switch(mode){
        case EditGain:
            mode = EditPan;
            break;
        case EditPan:
            mode = EditGain;
            break;
        }
        break;
    case 'q':case 'Q':
        throw _("Quit");
    default:break;
    }
}
