/**
 * @file value.cpp
 * @brief  Brief description of file.
 *
 */

#include <sstream>
#include "value.h"
#include "ctrl.h"
using namespace std;

vector<Value *> Value::values;


void Value::updateAll(){
    vector<Value *>::iterator it;
    for(it=values.begin();it!=values.end();it++){
        (*it)->update();
    }
};
              
string Value::toString(){
    stringstream ss;
    
    // options
    if(db)ss << "db ";
    
    if(fabsf(0.5f-smooth)>0.001f)
        ss << "smooth " << smooth << " ";
    
    if(optsset & VALOPTS_MIN)
        ss << "min " << mn << " ";
    if(optsset & VALOPTS_MAX)
        ss << "max " << mx << " ";
    
    // the value itself (with gain->db conversion if required)
    ss << (db?10.0f*log10f(target):target);
    
    if(ctrl){
        ss << "(" << ctrl->nameString << ") ";
    }
    
    return ss.str();
}
