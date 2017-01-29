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

void HelpScreen::display(MonitorData *h){
    showHelp(MainHelp);
}

void HelpScreen::flow(InputManager *im){
    im->getKey();
    im->pop();
}
