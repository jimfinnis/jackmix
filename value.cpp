/**
 * @file value.cpp
 * @brief  Brief description of file.
 *
 */

#include <sstream>
#include <algorithm>
#include "value.h"
#include "ctrl.h"
using namespace std;

vector<Value *> Value::values;

Value::~Value(){
    values.erase(std::remove(values.begin(),values.end(),this),values.end());
}

void Value::updateAll(){
    vector<Value *>::iterator it;
    for(it=values.begin();it!=values.end();it++){
        (*it)->update();
    }
};

void Value::removeCtrl(Ctrl *c){
    vector<Value *>::iterator it;
    for(it=values.begin();it!=values.end();it++){
        Value *v = *it;
        if(v->ctrl == c)v->ctrl=NULL;
    }
};

void Value::dump(){
    vector<Value *>::iterator it;
    for(it=values.begin();it!=values.end();it++){
        Value *v = *it;
        cout << v->name << endl;
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
    
    // the value itself
    ss << target;
    
    if(ctrl){
        ss << "(" << ctrl->nameString << ") ";
    }
    
    return ss.str();
}
