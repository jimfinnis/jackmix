/**
 * @file bounds.h
 * @brief  Brief description of file.
 *
 */

#ifndef __BOUNDS_H
#define __BOUNDS_H

/// a bounds structure which can have any of upper, lower bounds
/// and a default value. By default, has no information.
struct Bounds {
    Bounds(){
        flags=0;
    }
    
    float upper;
    float lower;
    float deflt;
    
    int flags;
    static const int Upper=1;
    static const int Lower=2;
    static const int Both=3;
    
    static const int Default=4;
    static const int Log=8;
};
    

#endif /* __BOUNDS_H */
