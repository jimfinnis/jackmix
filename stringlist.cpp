/**
 * @file stringlist.cpp
 * @brief  Brief description of file.
 *
 */

#include <algorithm>
#include <ncurses.h>

#include "stringlist.h"
#include "colours.h"

using namespace std;

static const int WIDTH=25; // not including LHS bar.

void StringList::display(){
    // this overwrites the right hand part of the display.
    
    int scrw,scrh;
    getmaxyx(stdscr,scrh,scrw);
    unsigned int x = scrw-WIDTH;
    unsigned int h = scrh-2; // room for filter line and status bar
    pagelen=h;
    
    // First draw the column.
    attrset(COLOR_PAIR(PAIR_GREEN));
    for(unsigned int y=0;y<h+1;y++) // extend past "bottom row" to cover filter line
        mvaddch(y,x-1,' ');
    attrset(COLOR_PAIR(0));
    
    
    if(cursor>0 && cursor>=listFiltered.size())
        cursor=listFiltered.size()-1; // negative won't happen (tested above)
    
    // integer divide/multiply to get page start
    int startpos = cursor/h;
    startpos *= h;
    
    for(unsigned int y=0;y<h;y++){
        unsigned int i = startpos+y;
        move(y,x);
        if(i < listFiltered.size()){
            attrset(i==cursor ? COLOR_PAIR(PAIR_HILIGHT)|A_BOLD:
                    COLOR_PAIR(0));
            addstr(listFiltered[i].substr(0,WIDTH-1).c_str());
        }
        clrtoeol();
    }
    
    attrset(COLOR_PAIR(0)|A_BOLD);
    mvaddstr(h+1,0,prompt.c_str()); // overwrites status line
    if(prefix.size())
        mvaddstr(h,x,prefix.c_str());
    attrset(COLOR_PAIR(0));
              
}

EditState StringList::handleKey(int k){
    unsigned int len = listFiltered.size();
    switch(k){
    case KEY_UP:
        if(cursor>0)cursor--;
        break;
    case KEY_DOWN:
        if(cursor<len-1)cursor++;
        break;
    case 10:
        state=Finished;
        break;
    case KEY_END:
    case 5: // ctrl-e
        cursor=len-1;
        break;
    case KEY_HOME:
    case 1: // ctrl-a
        cursor=0;
        break;
    case KEY_NPAGE:case 6: // ctrl-f
        if(len){
            cursor+=pagelen;
            if(cursor>len-1)cursor=len-1;
        }
        break;
    case KEY_PPAGE:case 2: // ctrl-b
        if(cursor>pagelen)
            cursor-=pagelen;
        break;
    case 7:
        state=Aborted;
        break;
    case 11: // ctrl-k
        prefix="";
        recalcFilter();
        break;
    case KEY_DC:case KEY_BACKSPACE:case 8:
        // shorten filter
        if(prefix.size())
            prefix.resize(prefix.size()-1);
        recalcFilter();
        break;
    default:
        if(k<128 && k>=32){
            prefix+=k;
            recalcFilter();
        }
        break;
    }
    return state;
        
}

void StringList::recalcFilter(){
    // try to keep the current string
    string cur;
    if(listFiltered.size() && cursor<listFiltered.size())
        cur = listFiltered[cursor];
    else
        cur = "";
    
    // do the filtering
    
    listFiltered.clear();
    
    vector<string>::iterator it;
    for(it = list.begin();it!=list.end();it++){
        if((*it).compare(0,prefix.size(),prefix)==0)
            listFiltered.push_back(*it);
    }
    
    // sort filtered list
    sort(listFiltered.begin(),listFiltered.end());
    
    // try to keep the current item
    
    if(cur.size()){
        for(unsigned int i=0;i<listFiltered.size();i++){
            if(listFiltered[i].compare(0,prefix.size(),prefix))
                cursor=i;
        }
    } else
        cursor=0;

}

