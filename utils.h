/**
 * @file utils.h
 * @brief  Audio utilities
 *
 */

#ifndef __UTILS_H
#define __UTILS_H

#include <iostream>
#include <string>

static const float PI = 3.1415927f;


// mono panning using a sin/cos taper to avoid the 6db drop at the centre
inline void panmono(float *__restrict left,
             float *__restrict right,
             float *__restrict in,
             float pan,float amp,int n){
    float ampl = cosf(pan*PI*0.5f);
    float ampr = sinf(pan*PI*0.5f);
    for(int i=0;i<n;i++){
        *left++ = *in * ampl;
        *right++ = *in++ * ampr;
    }
}

// stereo panning (balance) using a linear taper
inline void panstereo(float *__restrict leftout,
               float *__restrict rightout,
               float *__restrict leftin,
               float *__restrict rightin,
               float pan,float amp,int n){
    if(pan<0.5f){
        pan *= 2.0f;
        for(int i=0;i<n;i++){
            *leftout++ = *leftin++ * amp;
            *rightout++ = *rightin++ * pan * amp;
        }
    } else {
        pan = (1.0f-pan)*2.0f;
        for(int i=0;i<n;i++){
            *leftout++ = *leftin++ * pan * amp;
            *rightout++ = *rightin++ * amp;
        }
    }
        
}

inline void addbuffers(float *__restrict dest,
                       float *__restrict v, int n, float gain){
    for(int i=0;i<n;i++){
        *dest++ += gain**v++;
    }
}

class Monitor {
private:
    char *name;
    float cur;
    int iv;
public:
    int maxiv;
    Monitor(std::string n){
        name=strdup(n.c_str());
        cur=0;iv=0;maxiv=10;}
    Monitor(std::string n,int i){
        name=strdup(n.c_str());
        cur=0;iv=0;maxiv=i;}
    void in(float *v,int n){
        for(int i=0;i<n;i++){
            float q = fabs(*v++);
            cur = q*0.01f + cur*0.99f;
        }
    }
    float get(){return cur;}
};
    
    



#endif /* __UTILS_H */
