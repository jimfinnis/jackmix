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
    
    virtual ~ChainInterface(){}
    
    
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
    static ChainInterface *findornull(std::string name);
    
    static void zeroAllInputs();
    
    // run all effect chains in the order in which they were created.
    static void runAll(unsigned int n);
    
    // get names of all chains
    static std::vector<std::string> getNames();
    
    
    void save(std::ostream &out,std::string name);
    static void saveAll(std::ostream &out);
    static void addNewEmptyChain(std::string n);
    // delete chain by idx in chainlist
    static void deleteChain(int n);
    
    static void deleteEffect(int chainidx,int fidx);
    
    // generate a structure containing the connection and parameter data
    // for all fx in the chain, for editing. Messy, slightly, but it means
    // the actual fx gubbins stays encapsulated in fx.cpp.
    
    virtual struct ChainEditData *createEditData()=0;
    
    // add a new effect on the fly
    virtual void addEffect(PluginData *d,string name)=0;
    
    // remap an input on the fly (that poor fly)
    virtual void remapInput(std::string instname,
                            std::string inpname,
                            int chan, // 0/1 for chain inputs, -1 for another effect
                            // below used when chan==-1
                            std::string outinstname,
                            std::string outname)=0;
    
    // remap one of the chain's outputs to an effect output
    virtual void remapOutput(int outchan,
                             std::string instname,
                             std::string port)=0;
    
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
