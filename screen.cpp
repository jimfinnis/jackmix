/**
 * @file screen.cpp
 * @brief  general screen handling code
 *
 */

#include <vector>
#include <string>
#include <ncurses.h>
#include <string.h>

#include "screen.h"
#include "channel.h"
#include "monitor.h"
#include "ctrl.h"

void Screen::title(const char *s){
    attrset(COLOR_PAIR(0)|A_BOLD);
    int w = MonitorThread::get()->w;
    mvaddstr(0,w-strlen(s),s);
}


void Screen::drawVertBar(int y, int x, int h, int w, 
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

void Screen::drawHorzBar(int y, int x, int h, int w, 
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
    
    if(rv && rv->ctrl){
        char buf[64];
        sprintf(buf,"(%.20s)",rv->ctrl->nameString.c_str());
        attrset(COLOR_PAIR(0)|(bold?A_BOLD:0));
        mvaddstr(y+1,x+w-strlen(buf),buf);
    }
        
    
    for(int i=0;i<w;i++){
        int col;
        bool rev=true;
        switch(mode){
        case Gain:
            if(i>val){rev=true;
                col=PAIR_DARK;}
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
