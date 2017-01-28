/**
 * @file monitor.cpp
 * @brief  Brief description of file.
 *
 */

#include <sstream>
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#include "ringbuffer.h"
#include "exception.h"
#include "save.h"
#include "process.h"
#include "monitor.h"
#include "screen.h"
#include "version.h"
#include "colours.h"
#include "ctrl.h"

class Screen *curscreen=NULL; // the current screen. LOCK IT.

static InputRequest req;
MonitorThread *MonitorThread::instance =NULL;

/// primary mutex
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; 
/// condition used to block the main thread while we get input
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static pthread_t thread;

static void *_threadfunc(void *p){
    ((MonitorThread *)p)->threadfunc();
    return NULL;
}

// yes, these are all the same mutex
void MonitorThread::lock(){
    pthread_mutex_lock(&mutex);
}

void MonitorThread::unlock(){
    pthread_mutex_unlock(&mutex);
}

void InputManager::lock(){
    pthread_mutex_lock(&mutex);
}

void InputManager::unlock(){
    pthread_mutex_unlock(&mutex);
}
    

MonitorThread::MonitorThread(){
    if(instance)
        throw _("a monitor thread is already running.");
    
    running=true;requestStop=false;
    // start the thread
    
    pthread_create(&thread,NULL,_threadfunc,this);
    instance = this;
}

MonitorThread::~MonitorThread(){
    lock();
    requestStop=true;
    unlock();
    for(;;){
        usleep(10000);
        lock();
        if(!running)break;
        unlock();
    }
    instance = NULL;
    unlock();
}

void MonitorThread::threadfunc(){
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set,SIGINT);
    sigaddset(&set,SIGQUIT);
    pthread_sigmask(SIG_BLOCK,&set,NULL);
    
    // initialise curses
    
    initscr();
    keypad(stdscr,TRUE);
    noecho();
    cbreak();
    nl();
    curs_set(0);
    timeout(0); // nonblocking read
    
    
    // display initial status line
    stringstream ss;
    ss << "Jackmix Ready [" << VERSION << "]";
    setStatus(ss.str(),10);
    
    // set colour pairs
    start_color();
//    if(can_change_color())
//        setStatus("Color!",10);
    
    init_pair(PAIR_HILIGHT,COLOR_YELLOW,COLOR_BLACK);
    init_pair(PAIR_GREEN,COLOR_GREEN,COLOR_GREEN);
    init_pair(PAIR_YELLOW,COLOR_YELLOW,COLOR_YELLOW);
    init_pair(PAIR_RED,COLOR_RED,COLOR_RED);
    init_pair(PAIR_DARK,COLOR_BLUE,COLOR_BLUE);
    init_pair(PAIR_REDTEXT,COLOR_RED,COLOR_BLACK);
    init_pair(PAIR_BLUETEXT,COLOR_BLUE,COLOR_BLACK);
    
    loop(); // DO THE MAIN LOOP
    
    // shutdown curses
    endwin();
    
    printf("Terminated\n");
    running=false;
    unlock();
}

void MonitorThread::loop(){
    // main loop
    while(1){
        Screen *sc;
        getmaxyx(stdscr,h,w);
        lock();
        sc = curscreen;
        if(requestStop)break;
        // handle input requests
        if(!req.running && req.t!=InputReqIdle){
            // start a new one
            switch(req.t){
            case InputReqLineEdit:
                lineEdit.begin(req.prompt);
                break;
            case InputReqStringList:
                stringList.begin(req.prompt,req.list);
                break;
            default:break;
            }
            req.running=true;
        }
        unlock();
        
        // handle actual input
        int k = getch();
        if(req.running && k!=ERR){
            EditState newst;
            switch(req.t){
            case InputReqKey:
                if(req.validKeys.size()>0 && req.validKeys.find((char)k)==string::npos)
                    break; // abort if not a valid key
                lock();
                req.intout=k;
                req.setDone();
                // signal the other thread
                pthread_cond_signal(&cond);
                unlock();
                break;
            case InputReqLineEdit:
                newst=lineEdit.handleKey(k);
                if(newst!=Running){
                    lock();
                    req.aborted = newst==Aborted;
                    req.strout = lineEdit.consume();
                    req.setDone();
                    // signal the other thread
                    pthread_cond_signal(&cond);
                    unlock();
                }
                break;
            case InputReqStringList:
                newst=stringList.handleKey(k);
                if(newst!=Running){
                    lock();
                    req.aborted = newst==Aborted;
                    req.strout = stringList.consume();
                    req.setDone();
                    // signal the other thread
                    pthread_cond_signal(&cond);
                    unlock();
                }
                break;
            default:break;
            }
        }
        
        // fetch any data and display it using the current screen
        static MonitorData mdat;
        if(Process::pollMonRing(&mdat)){
            // we only erase sometimes, because it's only sometimes that data appears here
            erase();
            sc->display(&mdat);
        }
        
        // copy into a buffer for IM to look at (it should lock too)
        lock();
        lastReceived=mdat;
        unlock();
        
        // do the display, first the screen
        static unsigned int ct=0;
        mvprintw(0,40,"%ud",ct++);
        
        // display line edit if any, then string list if any, then keyprompt if any,
        // otherwise status 
        if(lineEdit.getState() == Running)
            lineEdit.display(h-1,0);
        else if(stringList.getState()==Running)
            stringList.display();
        else if(req.running && req.prompt.size()!=0){
            attrset(COLOR_PAIR(0)|A_BOLD);
            mvaddstr(h-1,0,req.prompt.c_str());
            attrset(COLOR_PAIR(0));
        } else
            // else the status msg.
            displayStatus();
        refresh();
        usleep(10000);
    }
}

void MonitorThread::showHelp(const char ***h){
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


void MonitorThread::setStatus(string s,double t){
    statusTimeToEnd=Time()+Time(t);
    statusMsg = s;
    statusShowing=true;
}


void MonitorThread::displayStatus(){
    if(statusShowing){
        Time now;
        if(now>statusTimeToEnd){
            statusShowing=false;
        } else {
            mvaddstr(h-1,0,statusMsg.c_str());
        }
    }
}

void InputManager::flow(){
    go(&scrMain);
    for(;;){
        curscreen->flow(this);
        if(!curscreen)return; // the screen did go(NULL)
        usleep(12000);
    }
}

void InputManager::setStatus(string s,double t){
    // this is called from the main thread, so we need
    // to lock.
    lock();
    MonitorThread::get()->setStatus(s,t);
    unlock();
}
    


string InputManager::getString(string p,bool *aborted){
    lock();
    // fill in the request
    req.startRequest(InputReqLineEdit);
    req.prompt=p;
    // unlock the mutex and wait for the condition
    pthread_cond_wait(&cond,&mutex);
    ///... some time later... the mutex will be locked
    
    *aborted = req.aborted;
    string rv = req.strout;
    
    unlock();
    return rv;
}
    
int InputManager::getKey(const char *p,const char *k){
    lock();
    req.startRequest(InputReqKey);
    req.prompt = p?p:"";
    if(k){
        stringstream ss;
        ss << req.prompt << " [" << k << "]";
        req.prompt = ss.str();
    }
    req.validKeys = k?k:"";
    pthread_cond_wait(&cond,&mutex);
    int rv = req.intout;
    unlock();
    return rv;
}

string InputManager::getFromList(string p,vector<string>& l,bool *aborted){
    lock();
    req.startRequest(InputReqStringList);
    req.prompt=p;
    req.list=l;
    pthread_cond_wait(&cond,&mutex);
    *aborted = req.aborted;
    string rv = req.strout;
    unlock();
    return rv;
}    
