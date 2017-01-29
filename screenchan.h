/**
 * @file screenchan.h
 * @brief  Brief description of file.
 *
 */

#ifndef __SCREENCHAN_H
#define __SCREENCHAN_H

#include "screen.h"

extern class ChanScreen : public Screen {
public:
    virtual void display(struct MonitorData *d);
    virtual void flow(class InputManager *im);
    
    void setChan(int n){
        chanidx=n;
    }
private:
    int chanidx=0; // indexes into the channel monitoring data array
    int curparam=0; // parameter being edited (first two are gain/pan, rest are send gains)
    
    void commandGainNudge(struct Channel *c,float v);
    void commandPanNudge(struct Channel *c,float v);
    void commandSendGainNudge(struct Channel *c,int send,float v);
} scrChan;


#endif /* __SCREENCHAN_H */
