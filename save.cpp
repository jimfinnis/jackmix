/**
 * @file save.cpp
 * @brief Does the reverse of the parser; writes config data
 * out to a file.
 *
 */

#include <string>
#include <fstream>
#include <sstream>
#include <iterator>
#include "channel.h"
#include "ctrl.h"

#include "exception.h"
#include "tokeniser.h"
#include "tokens.h"
#include "plugins.h"
#include "parser.h"
#include "save.h"

void saveMaster(ostream& out){
    extern Value *masterGain,*masterPan;
    out << "master gain " << masterGain->toString() << " ";
    out << "pan " << masterPan->toString() << endl;
}


void saveConfig(const char *fn){
    ofstream out;
    out.open(fn);
    
    saveMaster(out);
    
    Channel::saveAll(out);
    Ctrl::saveAll(out);
    ChainInterface::saveAll(out);
}


string intercalate(vector<string> v,const char *delim){
    ostringstream ss;
    switch(v.size()){
    case 0 : return "";
    case 1 : return v[0];
    default:
        std::copy(v.begin(),v.end()-1,
                  ostream_iterator<string>(ss,delim));
        ss << *v.rbegin();
        return ss.str();
    }
}
