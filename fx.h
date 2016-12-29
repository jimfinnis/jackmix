/**
 * @file fx.h
 * @brief Chains of LADSPA effects.
 *
 */

#ifndef __FX_H
#define __FX_H

#include <string.h>

#include "utils.h"

// the interface for FX chains. The chain itself inherits this and builds upon it.

struct ChainInterface {
    // these are the two input buffers for the chain
    float inpleft[BUFSIZE],inpright[BUFSIZE];
    // these are pointers to the output buffers for the two
    // output ports of the final effect
    float *leftoutbuf,*rightoutbuf;
    
    void zeroInputs(){
        memset(inpleft,0,BUFSIZE*sizeof(float));
        memset(inpright,0,BUFSIZE*sizeof(float));
    }
    
    // mono chains are currently implemented by adding to both the
    // left and right input 
    void addMono(float *v,float gain){
        addbuffers(inpleft,v,BUFSIZE,gain);
        addbuffers(inpright,v,BUFSIZE,gain);
    }
    
    void addStereo(float *left,float *right,float gain){
        addbuffers(inpleft,left,BUFSIZE,gain);
        addbuffers(inpright,right,BUFSIZE,gain);
    }
    
    // assumes all inputs are filled with data. Runs the fx in order
    // thus filling the output buffers.
    virtual void run(unsigned int nframes)=0;
    
    static ChainInterface *find(std::string name);
    
    static void zeroAllInputs();
    
    // run all effects in the order in which they were created.
    static void runAll(unsigned int n);
};



#endif /* __FX_H */
