/**
 * @file screenctrl.cpp
 * @brief  Brief description of file.
 *
 */

#include "monitor.h"
#include "process.h"
#include "proccmds.h"

#include "screenmain.h"
#include "screenctrl.h"

#include <ncurses.h>
#include <sstream>

CtrlScreen scrCtrl;

void CtrlScreen::display(MonitorData *d){
    title("CONTROLLER EDIT");
    attrset(COLOR_PAIR(0));
}

void CtrlScreen::flow(InputManager *im){
    
    int c = im->getKey();
    switch(c){
    case 'h':
        im->push();
        im->go(&scrHelp);
        break;
    case 'q':case 10:
        im->pop();
        break;
    default:break;
    }
}
