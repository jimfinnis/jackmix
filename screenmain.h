/**
 * @file screenmain.h
 * @brief  Brief description of file.
 *
 */

#ifndef __SCREENMAIN_H
#define __SCREENMAIN_H

#include "screen.h"

extern class MainScreen : public Screen {
public:
    virtual void display(struct MonitorData *d);
    virtual void flow(class InputManager *im);

private:
    int curchan=0;
    class Channel *curchanptr=NULL;
    void displayChan(int i,struct ChanMonData* c,bool cur); // c=NULL if invalid
    
    void commandGainNudge(float v);
    void commandPanNudge(float v);
    
} scrMain;


#endif /* __SCREENMAIN_H */
