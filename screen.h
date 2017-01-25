/**
 * @file screen.h
 * @brief  Brief description of file.
 *
 */

#ifndef __SCREEN_H
#define __SCREEN_H

#include <vector>
#include <string>
#include "colours.h"

/// vert/horz bar types
enum BarMode { Gain, // green/yellow/red
          Green, // all green
          Pan  // green, centered at 0.5
}; 


/// definition of a screen in the user interface.
/// flow() is run from the main thread. When it returns with NULL, the UI
/// and program will exit. Otherwise we go to the screen whose address is given
/// (which may be "this")
/// display() is run by the non-blocking UI thread every few milliseconds.

class Screen {
public:
    virtual void display(struct MonitorData *d)=0;
    virtual Screen *flow(class InputManager *im)=0;
protected:
    
    void title(const char *s);
    // v is 0-1 linear unless rv (range value) is present. We treat v and rv separately
    // so that we can store the value to show, passing it from process to main thread in
    // a ring buffer.
    void drawVertBar(int y, int x, int h, int w, 
                     float v,class Value *rv,BarMode mode,bool bold);
    void drawHorzBar(int y, int x, int h, int w, 
                     float v,class Value *rv,BarMode mode,bool bold);
};

extern class MainScreen : public Screen {
public:
    virtual void display(struct MonitorData *d);
    virtual Screen *flow(class InputManager *im);

private:
    int curchan=0;
    class Channel *curchanptr=NULL;
    void displayChan(int i,struct ChanMonData* c,bool cur); // c=NULL if invalid
    
    void commandGainNudge(float v);
    void commandPanNudge(float v);
    
} scrMain;


#endif /* __SCREEN_H */
