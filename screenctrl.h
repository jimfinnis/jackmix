/**
 * @file screenctrl.h
 * @brief  Brief description of file.
 *
 */

#ifndef __SCREENCTRL_H
#define __SCREENCTRL_H

#include "screen.h"

extern class CtrlScreen : public Screen {
public:
    virtual void display(struct MonitorData *d);
    virtual void flow(class InputManager *im);
    
private:
} scrCtrl;


#endif /* __SCREENCTRL_H */
