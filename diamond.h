/**
 * @file diamond.h
 * @brief  Brief description of file.
 *
 */

#ifndef __DIAMOND_H
#define __DIAMOND_H

#include "ctrlsource.h"

class DiamondSource : public CtrlSource {
public:
    virtual const char * add(std::string source,Ctrl *c);
    virtual void remove(Ctrl *c);
    virtual void setrangedefault(Ctrl *c){
        c->setinrange(0,1);
    }
    virtual const char *getName(){ return "diamond"; }
    
    void poll();
    void init();
};

struct DiamondSourceInfo : public CtrlSourceInfo {
    std::string topic;
    int index;
    
    DiamondSourceInfo(std::string t, int i){
        topic = t;
        index = i;
    }
};
    

extern DiamondSource diamond;

#endif /* __DIAMOND_H */
