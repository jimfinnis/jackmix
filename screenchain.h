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
    virtual void onEntry();
} scrChain;


#endif /* __SCREENCHAIN_H */
