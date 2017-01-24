/**
 * @file screen.h
 * @brief  Brief description of file.
 *
 */

#ifndef __SCREEN_H
#define __SCREEN_H

#include <vector>
#include <string>

/// definition of a screen in the user interface.
/// flow() is run from the main thread. When it returns with NULL, the UI
/// and program will exit. Otherwise we go to the screen whose address is given
/// (which may be "this")
/// display() is run by the non-blocking UI thread every few milliseconds.

class Screen {
public:
    virtual void display(struct MonitorData *d)=0;
    virtual Screen *flow(class InputManager *im)=0;
};

extern class MainScreen : public Screen {
    std::vector<std::string> list;
    virtual void display(struct MonitorData *d);
    virtual Screen *flow(class InputManager *im);
} scrMain;


#endif /* __SCREEN_H */
