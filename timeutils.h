/**
 * @file time.h
 * @brief Utilities for dealing with time.
 *
 */

#ifndef __TIME_H
#define __TIME_H

#include <time.h>

class Time {
    timespec t;
public:
    static const long BILLION = 1000000000;
    Time(){
        clock_gettime(CLOCK_MONOTONIC,&t);
    }
    
    Time(double tt){
        double secs;
        double frac = modf(tt,&secs);
        t.tv_sec = (long)secs;
        t.tv_nsec = (long)(frac*BILLION);
    }
    
    inline friend Time operator+(Time lhs,const Time& rhs){
        lhs.t.tv_sec += rhs.t.tv_sec;
        lhs.t.tv_nsec += rhs.t.tv_nsec;
        while(lhs.t.tv_nsec>BILLION){
            lhs.t.tv_nsec-=BILLION;
            lhs.t.tv_sec++;
        }
        return lhs;
    }
    
    inline friend double operator-(const Time& lhs, const Time& rhs){
        timespec temp;
        if ((lhs.t.tv_nsec-rhs.t.tv_nsec)<0) {
            temp.tv_sec = lhs.t.tv_sec-rhs.t.tv_sec-1;
            temp.tv_nsec = BILLION+lhs.t.tv_nsec-rhs.t.tv_nsec;
        } else {
            temp.tv_sec = lhs.t.tv_sec-rhs.t.tv_sec;
            temp.tv_nsec = lhs.t.tv_nsec-rhs.t.tv_nsec;
        }
    
        double d = temp.tv_sec;
        double ns = temp.tv_nsec;
        d += ns*1e-9;
        return d;
    }
    
    inline friend bool operator< (const Time& lhs, const Time& rhs){ 
        return (lhs-rhs)<0;
    }
    inline friend bool operator> (const Time& lhs, const Time& rhs){ return rhs < lhs; }
    inline friend bool operator<=(const Time& lhs, const Time& rhs){ return !(lhs > rhs); }
    inline friend bool operator>=(const Time& lhs, const Time& rhs){ return !(lhs < rhs); }
};


#endif /* __TIME_H */
