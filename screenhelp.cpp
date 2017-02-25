/**
 * @file screenhelp.cpp
 * @brief  Brief description of file.
 *
 */

#include "monitor.h"
#include "process.h"
#include "proccmds.h"

#include "screenmain.h"
#include "screenchain.h"

#include <sstream>
#include <ncurses.h>

#include "help.h"

HelpScreen scrHelp;

using namespace std;
vector<string> codestrings;

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
    
    if(!codestrings.empty()){
        int y=0;
        int x=80;
        attrset(COLOR_PAIR(PAIR_GREEN)|A_BOLD);
        mvprintw(y++,x,"Key codes");
        attrset(COLOR_PAIR(0));
        for(vector<string>::iterator it = codestrings.begin();
            it!=codestrings.end();it++){
            mvprintw(y++,x,(*it).c_str());
        }
    }
}

void HelpScreen::display(MonitorData *h){
    showHelp(MainHelp);
}

void HelpScreen::flow(InputManager *im){
    if(im->getKey()=='k'){
        int k = im->getKey("Press key to show its code");
        bool ab;
        string s = im->getString("An ID for this key",&ab);
        if(!ab){
            stringstream ss;
            ss << "ID " << s << ": " << k;
            codestrings.push_back(ss.str());
        }
    }else
          im->pop();
}
