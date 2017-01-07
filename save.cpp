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
#include "process.h"
#include "save.h"

using namespace std;

vector<string> pluginDirs;

void saveMaster(ostream& out){
    out << "master gain " << Process::masterGain->toString() << " ";
    out << "pan " << Process::masterPan->toString() << endl;
}


void saveConfig(const char *fn){
    ofstream out;
    out.open(fn);
    
    vector<string>::iterator it;
    for(it = pluginDirs.begin();it!=pluginDirs.end();it++){
        out << "plugindir \"" << *it << "\"\n";
    }
    
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
