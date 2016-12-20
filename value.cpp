/**
 * @file value.cpp
 * @brief  Brief description of file.
 *
 */

#include "value.h"

std::vector<Value *> Value::values;


void Value::updateAll(){
    std::vector<Value *>::iterator it;
    for(it=values.begin();it!=values.end();it++){
        (*it)->update();
    }
};
              
