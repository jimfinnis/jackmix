/**
 * @file lineedit.cpp
 * @brief  Brief description of file.
 *
 */

#include "lineedit.h"

#include <sstream>
#include <ncurses.h>

using namespace std;

void LineEdit::display(int y,int x){
    mvaddstr(y,x,prompt.c_str());
    addstr(": ");
    for(unsigned int i=0;i<data.size();i++){
        if(i==cursor)
            attrset(COLOR_PAIR(0)|A_BOLD|A_REVERSE);
        else
            attrset(COLOR_PAIR(0)|A_BOLD);
        addch(data[i]);
    }
    if(cursor==data.size()){
        attrset(COLOR_PAIR(0)|A_BOLD|A_REVERSE);
        addch(' ');
    }
    
        
}

LineEditState LineEdit::handleKey(int k){
    unsigned int len = data.size();
    switch(k){
    case 2: // ctrl-b
    case KEY_LEFT:
        if(cursor>0)cursor--;
        break;
    case 6: // ctrl-f
    case KEY_RIGHT:
        if(cursor<len)cursor++;
        break;
    case 10:
        state=Finished;
        break;
    case KEY_END:
    case 5: // ctrl-e
        cursor=len;
        break;
    case KEY_HOME:
    case 1: // ctrl-a
        cursor=0;
        break;
    case KEY_DC: // delete
        if(cursor<len)
            data.erase(cursor,1);
        break;
    case KEY_BACKSPACE:
    case 8: // backsp
        if(cursor>0){
            cursor--;
            data.erase(cursor,1);
        }
        break;
    case 7:
        state=Aborted;
        break;
    default:
        if(k<128 && k>=32){
            if(cursor<len)
                data.insert(cursor,1,(char)k);
            else
                data.append(1,(char)k);
            cursor++;
        }
        break;
    }
    return state;
}

