/**
 * @file plugins.h
 * @brief LADSPA management
 *
 */

#include <string>
#include <vector>
#include <unordered_map>

#include "ladspa.h"
#include "bounds.h"

#ifndef __PLUGINS_H
#define __PLUGINS_H


using namespace std;
// one per actual running plugin (i.e. instantiated and activated)
class PluginInstance {
    /// throw if a port is not connected, called by activate().
    /// Will only check control ports!
    void checkPortsConnected();
public:
    
    /// used for storing param->value associations for saving
    /// and editing. 
    unordered_map<string,class Value *> paramsMap;
    /// so we can access by integer
    vector<string> paramsList;
    
    struct PluginData *p;
    LADSPA_Handle h;
    string name; // the unique name, not the effect's label.
    bool isActive; // true if activate() has been called
    
    // which ports are set correctly - we check before running
    vector<bool> portsConnected;
    
    // maps output ports to their own buffer. Created in ctor.
    unordered_map<int,float*> opbufs;
    unordered_map<int,float*> connections; // for debugging snark
    
    // will instantiate, set default controls etc.
    PluginInstance(struct PluginData *plugin,string name,string chainname);
    // will deactivate if required/cleanup
    ~PluginInstance();
    
    // will activate the plugin
    void activate();
    
    // connect a port
    void connect(string name,float *v);
    
    // dump all ports to stdout
    void dump();
};

// one per plugin

struct PluginData {
    string label;
    const LADSPA_Descriptor *desc;
    unordered_map<string,int> shortPortNames;
    
    /// add a short name for a port's long name
    void addShortPortName(string shortname,string longname);
    
    // these are the values used as defaults for control ports
    // for this plugin.
    unordered_map<unsigned int,float> defaultPortValues;
    
    /// try to find the index of a plugin's port by long or short name
    int getPortIdx(string name);
    
    /// get the default for a parameter, returning false if none.
    bool getDefault(string pname,float *f);
    
    /// get the bounds for a port
    Bounds getBounds(string pname);
    
    PluginData(string l,const LADSPA_Descriptor *d);
    
    
    
    PluginInstance *instantiate(string name,string chainname);
};


    

/// This is a namespace which manages all LADSPA plugins.
/// It can load a file and work out the parameters, and provides
/// a facility for short names to be assigned to the parameters.

namespace PluginMgr {
/// scan a plugin directory and create entries for all plugins
/// by label (but do not load them).
void loadFilesIn(const char *dir,bool permitNoDir=false);
/// find a plugin, throwing if not found
PluginData *getPlugin(std::string label);
/// delete all instances - AFTER stopping the process thread!
void deleteInstances();
/// close anything left over
void close();
/// list of all plugins loaded
extern std::vector<std::string> pluginNames;

}

#endif /* __PLUGINS_H */
