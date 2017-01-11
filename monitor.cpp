/**
 * @file monitor.cpp
 * @brief  Brief description of file.
 *
 */

#include <sstream>
#include <ncurses.h>

#include "ringbuffer.h"
#include "exception.h"
#include "save.h"
#include "process.h"
#include "monitor.h"
#include "version.h"
#include "colours.h"


#include "help.h"

static bool inHelp=false;

void MonitorUI::setStatus(string s,double t){
    statusTimeToEnd=Time()+Time(t);
    statusMsg = s;
    statusShowing=true;
}


void MonitorUI::displayStatus(){
    if(statusShowing){
        Time now;
        if(now>statusTimeToEnd){
            statusShowing=false;
        } else {
            mvaddstr(h-1,0,statusMsg.c_str());
        }
    }
}

MonitorUI::MonitorUI(){
    initscr();
    keypad(stdscr,TRUE);
    noecho();
    cbreak();
    nl();
    curs_set(0);
    state=Main;
    prevState=Main;
    timeout(0); // nonblocking read
    
    
    // display initial status line
    stringstream ss;
    ss << "Jackmix Ready [" << VERSION << "]";
    setStatus(ss.str(),10);
    
    // set colour pairs
    start_color();
    if(can_change_color())
        setStatus("Color!",10);
    
    init_pair(PAIR_HILIGHT,COLOR_YELLOW,COLOR_BLACK);
    init_pair(PAIR_GREEN,COLOR_GREEN,COLOR_GREEN);
    init_pair(PAIR_YELLOW,COLOR_YELLOW,COLOR_YELLOW);
    init_pair(PAIR_RED,COLOR_RED,COLOR_RED);
    init_pair(PAIR_DARK,COLOR_BLUE,COLOR_BLUE);
    init_pair(PAIR_REDTEXT,COLOR_RED,COLOR_BLACK);
    init_pair(PAIR_BLUETEXT,COLOR_BLUE,COLOR_BLACK);
    
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


/*
 * 
 * 
 * States
 * 
 * 
 * 
 */

static void showHelp(const char ***h){
    attrset(COLOR_PAIR(0));
    for(int x=0;h[x];x++){
        for(int y=0;h[x][y];y++){
            move(y,x*40);
            const char *s = h[x][y];
            while(*s){
                switch(*s){
                case '[':attron(A_BOLD);break;
                case ']':attroff(A_BOLD);break;
                case '{':attron(COLOR_PAIR(PAIR_HILIGHT));break;
                case '}':attroff(COLOR_PAIR(PAIR_HILIGHT));break;
                default:
                    addch(*s);
                    break;
                }
                s++;
            }
        }
    }
}


static MonitorData lastDisplayed;


#define RIGHTWIDTH 20
#define COLWIDTH 10

void MonitorUI::displayMain(MonitorData *d){
    title("MAIN VIEW");
    // how many channels (cols) can we do, not including MASTER?
    // There's a bit of space over
    // to the right we need to leave free.
    
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
        displayChan(i+1,c,chanidx==curchan);
    }
}

void MonitorUI::displayChan(int i,ChanMonData* c,bool cur){
    int x = i*COLWIDTH;
    
    float l,r,gain,pan;
    const char *name;
    
    if(c){
        l=c->l;
        r=c->r;
        gain=c->gain;
        pan=c->pan;
        name = c->name;
        if(c->chan){
            if(c->chan->isMute()){
                attrset(COLOR_PAIR(PAIR_BLUETEXT)|A_BOLD);
                mvaddstr(1,x,"MUTE");
            }
            if(c->chan->isSolo()){
                attrset(COLOR_PAIR(PAIR_REDTEXT)|A_BOLD);
                mvaddstr(1,x+5,"SOLO");
            }
        }
    } else {
        l=r=gain=pan=0;
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
    drawVertBar(2,x+4,h-3,1,gain,NULL,Green,cur);
    drawVertBar(2,x+6,h-3,1,pan,NULL,Pan,cur);
    
    attrset(COLOR_PAIR(0));
}


void MonitorUI::displayChanZoom(MonitorData *d){
    title("CHANNEL EDIT");
    attrset(COLOR_PAIR(PAIR_HILIGHT)|A_BOLD);
    if(curchan<0 || curchan>=d->numchans){
        mvprintw(0,0,"Invalid channel");
        curchanptr = NULL;
    } else {
        ChanMonData *c = d->chans+curchan;
        curchanptr = c->chan;
        mvprintw(0,0,"Channel %s",c->name);
        
        int numsends = (int)curchanptr->chains.size();
        // it's ugly checking this here, but it's a safeish
        // and convenient place
        if(curparam-2 >= numsends)
            curparam = 1+numsends;
        
        cursend = curparam-2; // currently edited send
        
        
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
        drawHorzBar(8,10,1,ww,c->gain,NULL,Green,curparam==0);
        drawHorzBar(10,10,1,ww,c->pan,NULL,Pan,curparam==1);
        
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
    refresh();
}

// the chain we are currently viewing
static ChainEditData *chainData=NULL;
// the effect within the chain we are currently viewing
static int cureffect=0;
// the parameter within the effect we are currently editing
static int cureffectparam=0;

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

void MonitorUI::displayChainList(){
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
        attrset((int)i==curparam ? chainHilight : COLOR_PAIR(0));
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



/*
 * 
 * Drawing utils
 * 
 */

void MonitorUI::title(const char *s){
    attrset(COLOR_PAIR(0)|A_BOLD);
    mvaddstr(0,w-strlen(s),s);
}

void MonitorUI::drawVertBar(int y, int x, int h, int w, 
                            float v,Value *rv,BarMode mode,bool bold){
    float mn,mx;
    if(rv){
        mn = rv->mn; mx = rv->mx;
        // we only do decibel conversion of the range
        if(rv->db){
            mn = powf(10.0f,mn*0.1f);
            mx = powf(10.0f,mx*0.1f);
        }
    } else {
        mn=0;mx=1;
    }
    
    
    
    // we DON'T do decibel conversion.
    v -= mn;
    v /= (mx-mn);
    
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
                col=PAIR_DARK;rev=false;}
            else if(i>red)
                col=PAIR_RED;
            else if(i>yellow)
                col=PAIR_YELLOW;
            else 
                col=PAIR_GREEN;
            break;
        case Pan:
            if(val<half){
                if(i>half || i<val){col=PAIR_DARK;rev=false;}
                else col=PAIR_GREEN;
            } else {
                if(i<half || i>val){col=PAIR_DARK;rev=false;}
                else col=PAIR_GREEN;
            }
            break;
        case Green:
            if(i>val){col=PAIR_DARK;rev=false;}
            else col = PAIR_GREEN;
        }
        long attr = bold ? A_BOLD: 0;
        if(rev)attr|=A_REVERSE;
        attrset(COLOR_PAIR(col) | attr);
        mvaddch(h-(y+i)+3,x,' ');
        for(int j=1;j<w;j++)addch(' ');
    }
}

void MonitorUI::drawHorzBar(int y, int x, int h, int w, 
                            float v,Value *rv,BarMode mode,bool bold){
    float mn,mx;
    if(rv){
        mn = rv->mn; mx = rv->mx;
        // we only do decibel conversion of the range
        if(rv->db){
            mn = powf(10.0f,mn*0.1f);
            mx = powf(10.0f,mx*0.1f);
        }
    } else {
        mn=0;mx=1;
    }
    
    // we DON'T do decibel conversion.
    v -= mn;
    v /= (mx-mn);
    
    int val = v*w;
    int yellow = w*0.7f;
    int red = w*0.9f;
    int half = w/2;
    
    for(int i=0;i<w;i++){
        int col;
        bool rev=true;
        switch(mode){
        case Gain:
            if(i>val){
                col=PAIR_DARK;rev=false;}
            else if(i>red)
                col=PAIR_RED;
            else if(i>yellow)
                col=PAIR_YELLOW;
            else 
                col=PAIR_GREEN;
            break;
        case Pan:
            if(val<half){
                if(i>half || i<val){col=PAIR_DARK;rev=false;}
                else col=PAIR_GREEN;
            } else {
                if(i<half || i>val){col=PAIR_DARK;rev=false;}
                else col=PAIR_GREEN;
            }
            break;
        case Green:
            if(i>val){col=PAIR_DARK;rev=false;}
            else col = PAIR_GREEN;
        }
        long attr = bold ? A_BOLD: 0;
        if(rev)attr|=A_REVERSE;
        attrset(COLOR_PAIR(col) | attr);
        for(int j=0;j<h;j++)
            mvaddch(y+j,x+i,' ');
    }
}




/*
 * 
 * Commands to process thread
 * 
 */


void MonitorUI::command(MonitorCommandType cmd,float v,Channel *c,int i){
    Process::writeCmd(cmd,v,c,i);
}

void MonitorUI::commandGainNudge(float v){
    if(curchanptr){
        if(curchanptr->gain->db)v*=0.1f;
        command(MonitorCommandType::ChangeGain,v,curchanptr);
    } else {
        v*=0.1f; // master gain is always log
        command(MonitorCommandType::ChangeMasterGain,v);
    }
}    

void MonitorUI::commandPanNudge(float v){
    if(curchanptr){
        command(MonitorCommandType::ChangePan,v,curchanptr);
    } else 
        command(MonitorCommandType::ChangeMasterPan,v,NULL);
}

void MonitorUI::commandSendGainNudge(float v){
    if(curchanptr){
        if(cursend < (int)curchanptr->chains.size()){
            ChainFeed& f = curchanptr->chains[cursend];
            if(f.gain->db)v*=0.1f;
            command(MonitorCommandType::ChangeSendGain,v,
                    curchanptr,cursend);
        }
    }    
}

void MonitorUI::commandParamNudge(float v){
    if(chainData && cureffect>=0 && cureffect<(int)chainData->fx.size()){
        PluginInstance *fx;
        if(cureffect>=0 && cureffect<(int)chainData->fx.size())
            fx = chainData->fx[cureffect];
        else
            fx = NULL;
        
        if(fx && cureffectparam>=0 &&
           cureffectparam<(int)fx->paramsList.size()){
            Value *p = fx->paramsMap[fx->paramsList[cureffectparam]];
            Process::writeCmd(MonitorCommand(MonitorCommandType::ChangeEffectParam,
                                             v,p));
        }
    }
     
}

void MonitorUI::simpleChannelCommand(MonitorCommandType cmd){
    if(curchanptr){
        command(cmd,0,curchanptr);
    }
}


void MonitorUI::display(MonitorData *d){
    getmaxyx(stdscr,h,w);
    if(!d)d=gentestdat();
    erase();
    
    
    helpScreen = MainHelp;
    
    if(inHelp){
        title("HELP");
        showHelp(helpScreen);
    } else {
        switch(state){
        case Main:
            displayMain(d);
            break;
        case ChanZoom:
            displayChanZoom(d);
            break;
        case ChainList:
            displayChainList();
            break;
        default:
            mvaddstr(0,0,"????");
            break;
        }
    }
    
    if(keyMethod){
        attrset(COLOR_PAIR(0)|A_BOLD);
        mvaddstr(h-1,0,keyString.c_str());
    } else if(stringList.getState()==Running)
        stringList.display();
    else if(lineEdit.getState()==Running)
        lineEdit.display(h-1,0);
    else
        displayStatus();
    
    lastDisplayed=*d;
    
    refresh();
}

// callbacks for line editing or key get

void MonitorUI::lineFinishedSaveFile(std::string s){
    saveConfig(s.c_str());
    setStatus("Saved.",2);
}

void MonitorUI::confirmDeleteSend(int key){
    if(key == 'y' || key == 'Y'){
        if(curchanptr){
            Process::writeCmd(MonitorCommand(
                                             MonitorCommandType::DelSend,
                                             0,curchanptr,
                                             cursend));
        }
    }
}



void MonitorUI::handleInput(){
    PluginInstance *fx;
    
    if(keyMethod!=NULL){
        int c = getch();
        if(c!=ERR){
            (this->*keyMethod)(c);
            keyMethod=NULL;
        }
    } else if(stringList.getState()==Running){
        stringList.handleKey(getch());
        switch(stringList.getState()){
        case Aborted:
            stringList.consume();
            break;
        case Finished:
            (this->*stringFinishedMethod)(stringList.consume());
            break;
        default:break;
        }
    } else if(lineEdit.getState()==Running){
        lineEdit.handleKey(getch());
        // handle line editor returne
        switch(lineEdit.getState()){
        case Aborted:
            lineEdit.consume();
            break;
        case Finished:
            (this->*lineFinishedMethod)(lineEdit.consume());
            break;
        default:break;
        }
    } else if(inHelp){
        if(getch()!=ERR)inHelp=false;
    } else {
        int c = getch();
        if(c=='h'||c=='H')inHelp=true;
        switch(state){
        case ChanZoom:
            switch(c){
            case 'a':case 'A':
#pragma message "Show a list of chains here to select from"
                {
                    vector<string> test;
test.push_back("tuneful");
test.push_back("tunefully");
test.push_back("tuneless");
test.push_back("tunelessly");
test.push_back("tuner");
test.push_back("tuner's");
test.push_back("tuners");
test.push_back("tunes");
test.push_back("tungsten");
test.push_back("tungsten's");
test.push_back("tunic");
test.push_back("tunic's");
test.push_back("tunics");
test.push_back("tuning");
test.push_back("tunnel");
test.push_back("tunnel's");
test.push_back("tunneled");
test.push_back("tunneling");
test.push_back("tunnelings");
test.push_back("tunnelled");
test.push_back("tunnelling");
test.push_back("tunnels");
test.push_back("tunnies");
test.push_back("tunny");
test.push_back("tunny's");
test.push_back("tuns");
test.push_back("turban");
test.push_back("turban's");
test.push_back("turbans");
test.push_back("turbid");
test.push_back("turbine");
test.push_back("turbine's");
test.push_back("turbines");
test.push_back("turbojet");
test.push_back("turbojet's");
test.push_back("turbojets");
test.push_back("turboprop");
test.push_back("turboprop's");
test.push_back("turboprops");
test.push_back("turbot");
test.push_back("turbot's");
test.push_back("turbots");
test.push_back("turbulence");
test.push_back("turbulence's");
test.push_back("turbulent");
test.push_back("turbulently");
test.push_back("turd");
test.push_back("turd's");
test.push_back("turds");
test.push_back("tureen");
test.push_back("tureen's");
test.push_back("tureens");
test.push_back("turf");
test.push_back("turf's");
test.push_back("turfed");
test.push_back("turfing");
test.push_back("turfs");
test.push_back("turgid");
test.push_back("turgidity");
test.push_back("turgidity's");
test.push_back("turgidly");
test.push_back("turkey");
test.push_back("turkey's");
test.push_back("turkeys");
test.push_back("turmeric");
test.push_back("turmeric's");
test.push_back("turmerics");
test.push_back("turmoil");
test.push_back("turmoil's");
test.push_back("turmoils");
test.push_back("turn");
test.push_back("turn's");
test.push_back("turnabout");
test.push_back("turnabout's");
test.push_back("turnabouts");
test.push_back("turnaround");
test.push_back("turnaround's");
test.push_back("turnarounds");
test.push_back("turncoat");
test.push_back("turncoat's");
test.push_back("turncoats");
test.push_back("turned");
test.push_back("turner");
test.push_back("turner's");
test.push_back("turners");
test.push_back("turning");
test.push_back("turnip");
test.push_back("turnip's");
test.push_back("turnips");
test.push_back("turnkey");
test.push_back("turnkey's");
test.push_back("turnkeys");
test.push_back("turnoff");
test.push_back("turnoff's");
test.push_back("turnoffs");
test.push_back("turnout");
test.push_back("turnout's");
test.push_back("turnouts");
test.push_back("turnover");
test.push_back("turnover's");
test.push_back("turnovers");
test.push_back("turnpike");
test.push_back("turnpike's");
test.push_back("turnpikes");
test.push_back("turns");
test.push_back("turnstile");
test.push_back("turnstile's");
test.push_back("turnstiles");
test.push_back("turntable");
test.push_back("turntable's");
test.push_back("turntables");
test.push_back("turpentine");
test.push_back("turpentine's");
test.push_back("turpitude");
test.push_back("turpitude's");
test.push_back("turquoise");
test.push_back("turquoise's");
test.push_back("turquoises");
test.push_back("turret");
test.push_back("turret's");
test.push_back("turrets");
test.push_back("turtle");
test.push_back("turtle's");
test.push_back("turtledove");
test.push_back("turtledove's");
test.push_back("turtledoves");
test.push_back("turtleneck");
test.push_back("turtleneck's");
test.push_back("turtlenecks");
test.push_back("turtles");
                    beginStringList("string",test,&MonitorUI::stringFinishedAddChain);
                }
                break;
            case KEY_DC: // delete send
                if(cursend>=0)
                    getKey("Delete send: are you sure (y/N)?",&MonitorUI::confirmDeleteSend);
                break;
            case 'p':case 'P':
                if(curchanptr && cursend>=0){
                    Process::writeCmd(MonitorCommand(
                                                     MonitorCommandType::TogglePrePost,
                                                     0,curchanptr,
                                                     cursend));
                }
                break;
            case 'q':case 'Q':
            case 10: gotoPrevState();break;
            case 'm':case 'M':
                simpleChannelCommand(MonitorCommandType::ChannelMute);
                break;
            case 's':case 'S':
                simpleChannelCommand(MonitorCommandType::ChannelSolo);
                break;
            case KEY_UP:
                if(--curparam < 0)curparam=0;
                break;
            case KEY_DOWN:
                curparam++; // out-of-range dealt with by display
                break;
            case KEY_LEFT:
                switch(curparam){
                case 0:commandGainNudge(-1);break;
                case 1:commandPanNudge(-1);break;
                default:commandSendGainNudge(-1);break;
                }
                break;
            case KEY_RIGHT:
                switch(curparam){
                case 0:commandGainNudge(1);break;
                case 1:commandPanNudge(1);break;
                default:commandSendGainNudge(1);break;
                }
                break;
            default:break;
            }
            break;
        case ChainList:
            switch(c){
            case 10:
                gotoPrevState();
                break;
            case 9: // tab
                switch(chainListMode){
                case Chain:chainListMode=Effects;break;
                case Effects:chainListMode=Params;break;
                case Params:chainListMode=Chain;break;
                }
                break;
            case KEY_UP:
                switch(chainListMode){
                case Chain:curparam--;
                    if(curparam<0)curparam=((int)chainlist.size())-1;
                    regenChainData(curparam);
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
                break;
            case KEY_DOWN:
                switch(chainListMode){
                case Chain:curparam++;
                    if(curparam>=(int)chainlist.size())
                        curparam=0;
                    regenChainData(curparam);
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
            break;
        case Main:
            switch(c){
            case 'w':
                beginLineEdit("Filename",&MonitorUI::lineFinishedSaveFile);
                break;
            case 'c':case 'C':
                gotoState(ChainList);
                regenChainData(curparam);
                break;
            case 'm':case 'M':
                simpleChannelCommand(MonitorCommandType::ChannelMute);
                break;
            case 's':case 'S':
                simpleChannelCommand(MonitorCommandType::ChannelSolo);
                break;
            case 10:
                if(curchan>=0)
                    gotoState(ChanZoom);
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
                throw _("Quit");
            default:break;
            }
        default:break;
        }
    }
}
