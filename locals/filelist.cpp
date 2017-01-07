/**
 * @file filelist.cpp
 * @brief  Brief description of file.
 *
 */

#include "ladspa.h"

// add them up here

extern "C"{
const LADSPA_Descriptor *LOCALS_ladspa_descriptor(unsigned long index);
}

const LADSPA_Descriptor *getLocal(unsigned long i,unsigned long j){
    // and down here
    switch(i){
    case 0:
        return LOCALS_ladspa_descriptor(j);
    default:
        return (LADSPA_Descriptor*)0L;
    }
}
