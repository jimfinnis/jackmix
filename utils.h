/**
 * @file utils.h
 * @brief  Audio utilities
 *
 */

#ifndef __UTILS_H
#define __UTILS_H

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
            *leftout++ = *leftin++;
            *rightout++ = *rightin++ * pan;
        }
    } else {
        pan = (1.0f-pan)*2.0f;
        for(int i=0;i<n;i++){
            *leftout++ = *leftin++ * pan;
            *rightout++ = *rightin++;
        }
    }
        
}


#endif /* __UTILS_H */
