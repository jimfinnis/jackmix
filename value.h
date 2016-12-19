/**
 * @file value.h
 * @brief  Brief description of file.
 *
 */

#ifndef __VALUE_H
#define __VALUE_H

// these are the values used in the mixer: pan positions, gains etc.
// and effect parameters. They can either be constants, or have a default
// value but be modifiable from a control channel.

class Value {
    /// the post-conversion default value
    float deflt;
    
public:
    Value(){
        deflt=0;
    }
    
    /// the current value
    float value;
    
    Value *setdef(float def){
        deflt=def;
        value=def;
        return this;
    }
    
    /// reset to default
    void reset(){
        value=deflt;
    }
          
};   
                
        
        
    
    


#endif /* __VALUE_H */
