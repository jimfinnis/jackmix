/**
 * @file ctrlsource.h
 * @brief  Singleton abstract which manages sources of control data,
 * and the data type for the source data inside the ctrl.
 *
 */

#ifndef __CTRLSOURCE_H
#define __CTRLSOURCE_H

#include "ctrl.h"


class CtrlSource {
public:
    virtual const char * add(std::string spec,Ctrl *c)=0;
    virtual void remove(Ctrl *c)=0;;
    virtual void setrangedefault(Ctrl *c)=0;
    virtual const char *getName()=0;
};

// Data associated with the source that's stored in the ctrl;
// empty, but we need to inherit from it. I would bolt this data into subclasses of
// the ctrl, but I might need to change a ctrl type on the fly.

class CtrlSourceInfo {
    
};

#endif /* __CTRLSOURCE_H */
