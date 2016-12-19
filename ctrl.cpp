/**
 * @file ctrl.cpp
 * @brief  Brief description of file.
 *
 */
#include "ctrl.h"

std::unordered_map<std::string,Ctrl *> Ctrl::map;

Ctrl *Ctrl::createOrFind(std::string name){
    Ctrl *v;
    std::unordered_map<std::string,Ctrl *>::iterator res;
    res = map.find(name);
    if(res==map.end()){
        v = new Ctrl();
        map.emplace(name,v);
    } else {
        v = res->second;
    }
    return v;
}
