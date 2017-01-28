/**
 * @file screenchain.h
 * @brief  Brief description of file.
 *
 */

#ifndef __SCREENCHAIN_H
#define __SCREENCHAIN_H

#include "screen.h"

extern class ChainScreen : public Screen {
public:
    virtual void display(struct MonitorData *d);
    virtual void flow(class InputManager *im);
    
private:
    int chanidx=0; // indexes into the channel monitoring data array
    int curparam=0; // parameter being edited (first two are gain/pan, rest are send gains)
} scrChain;


#endif /* __SCREENCHAIN_H */
