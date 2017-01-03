/**
 * @file fx.h
 * @brief Chains of LADSPA effects.
 *
 */

#ifndef __FX_H
#define __FX_H

#include <string.h>

#include "utils.h"
#include "plugins.h"

// the interface for FX chains. The chain itself inherits this and builds upon it.

struct ChainInterface {
    std::string name; // name for viewing
    
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
    
    
    void save(std::ostream &out,std::string name);
    static void saveAll(std::ostream &out);
    
    // generate a structure containing the connection and parameter data
    // for all fx in the chain, for editing. Messy, slightly, but it means
    // the actual fx gubbins stays encapsulated in fx.cpp.
    
    virtual struct ChainEditData *createEditData()=0;
    
};

extern std::vector<ChainInterface *> chainlist;


// stores data about an input connection for an effect. This isn't
// used in processing, but is used in saving and editing.

struct InputConnectionData {
    int port; // the port for this input
    
    // and where it comes from:
    int channel; // -1 if this is an internal connection, 0=left,1=right
    // if channel=-1, info about the effect this comes from
    std::string fromeffect,fromport;
};

// editor data used by the monitor.

struct ChainEditData {
    // a list of all the effects 
    std::vector<PluginInstance *> fx;
    // names of the output connections from the chain
    std::string leftouteffect,leftoutport;
    std::string rightouteffect,rightoutport;
    std::vector<std::vector<InputConnectionData>*> *inputConnData;
};


#endif /* __FX_H */
