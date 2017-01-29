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
/// flow() is run from the main thread an just runs over and over.
/// display() is run by the non-blocking UI thread every few milliseconds.

class Screen {
public:
    virtual void display(struct MonitorData *d)=0;
    virtual void flow(class InputManager *im)=0;
    virtual void onEntry(){}
protected:
    
    void title(const char *s);
    void setStatus(const char *s,double t=2);
    // v is 0-1 linear unless rv (range value) is present. We treat v and rv separately
    // so that we can store the value to show, passing it from process to main thread in
    // a ring buffer.
    void drawVertBar(int y, int x, int h, int w, 
                     float v,class Value *rv,BarMode mode,bool bold);
    void drawHorzBar(int y, int x, int h, int w, 
                     float v,class Value *rv,BarMode mode,bool bold);
};

#include "screenhelp.h"


#endif /* __SCREEN_H */
