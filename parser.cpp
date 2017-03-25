/**
 * @file parser.cpp
 * @brief Config file parsing code.
 *
 */

#include <string>
#include "channel.h"
#include "ctrl.h"

#include "exception.h"
#include "tokeniser.h"
#include "tokens.h"
#include "plugins.h"
#include "process.h"
#include "parser.h"

Tokeniser tok;

// values are <number>['('<ctrl>')'], where <number> might be "default"
// bounds: a Bounds structure containing an upper and/or lower bound,
// which cannot be overriden with min/max (used in effects). Can parse
// into existing or new value.
Value *parseValue(Bounds b,Value *v=NULL)
{
    if(!v)
        v = new Value("");
    
    float rmin=0,rmax=1;
    float smooth = 0.5;
    
    v->optsset=0;
    
    for(;;){
        switch(tok.getnext()){
        case T_DB:
            v->setdb();
            // might get overwritten later..
            rmin=MINDB;rmax=MAXDB;
            break;
        case T_MIN:
            if(b.flags & Bounds::Lower)
                throw _("'min' not permitted, min is fixed to %f",b.lower);
            rmin = getnextfloat();
            v->optsset|=VALOPTS_MIN;
            break;
        case T_MAX:
            if(b.flags & Bounds::Upper)
                throw _("'max' not permitted, max is fixed to %f",b.upper);
            rmax = getnextfloat();
            v->optsset|=VALOPTS_MAX;
            break;
        case T_SMOOTH:
            smooth = tok.getnextfloat();
            if(tok.iserror())expected("number");
            break;
        case T_DEFAULT:
            if(!(b.flags & Bounds::Default))
                throw _("no default is provided for this value");
        case T_INT:
        case T_FLOAT:
            goto optsdone;
        default:
            expected("number or value option");
        }
    }
    
optsdone:
    
    if(b.flags & Bounds::Upper)
        rmax = b.upper;
    if(b.flags & Bounds::Lower)
        rmin = b.lower;
    
    // use the default value passed in if we have one and it's allowed
    float n;
    if(tok.getcurrent() == T_DEFAULT){ // checked above
        n = b.deflt;
    } else if(tok.getcurrent() == T_FLOAT || tok.getcurrent()==T_INT)
        n = tok.getfloat();
    else
        expected("number or 'default'");
    
    v->setdef(n);
    v->setrange(rmin,rmax);
    v->reset();
    
    if(tok.getnext()==T_OPREN){
        // there is a controller for this value!
        string name = getnextident();
        Ctrl *c = Ctrl::createOrFind(name);
        c->addval(v);
        v->ctrl = c;
        
        if(tok.getnext()==T_SMOOTH){
            n = tok.getnextfloat();
            if(tok.iserror())expected("number");
        } else tok.rewind();
        
        if(tok.getnext()!=T_CPREN)
            expected("')'");
    } else
        tok.rewind();
    v->setsmooth(smooth);
    return v;
}

void parseChan(){
    string name=getnextident();
    
    if(tok.getnext()!=T_COLON)
        expected(":");
    
    printf("Parsing channel %s\n",name.c_str());
    
    string returnChainName;
    bool isReturn=false;
    if(tok.getnext()==T_RETURN){
        isReturn=true;
        returnChainName = getnextident();
    }else tok.rewind();
    
    if(tok.getnext()!=T_GAIN)
        expected("'gain'");
    Value *gain = parseValue(Bounds());
    gain->setname(name+" gain");
    
    if(tok.getnext()!=T_PAN)
        expected("'pan'");
    Value *pan = parseValue(Bounds());
    pan->setname(name+" pan");
    
    bool mono=false;
    switch(tok.getnext()){
    case T_MONO:mono=true;
    case T_STEREO:break;
    default:tok.rewind();break;
    }
    
    // can now create the channel. The Channel class maintains
    // a static list of channels to which the ctor will add the
    // new one.
    Channel *ch = new Channel(name,mono?1:2,gain,pan,isReturn,
                              returnChainName);
    
    // add info about chains, which will be resolved later.
    while(tok.getnext()==T_SEND){
        string chain = getnextident();
        if(tok.getnext()!=T_GAIN)expected("'gain'");
        Value *chaingain=parseValue(Bounds());
        chaingain->setname(name+"->"+chain+" gain");
        
        int t = tok.getnext();
        bool postfade=false;
        if(t==T_POSTFADE)postfade=true;
        else if(t!=T_PREFADE)tok.rewind();
        
        ch->addChainInfo(chain,chaingain,postfade);
    }
    tok.rewind(); // should leave us at a comma
    
    
}

void parseCtrl(){
    string name=getnextident();
    
    if(tok.getnext()!=T_COLON)
        expected("':' after ctrl name");
    
    if(tok.getnext()!=T_STRING)
        expected("string (ctrl source spec)");
    
    string spec = string(tok.getstring());
    
    // creating the ctrl will set the default ranges
    
    Ctrl *c = Ctrl::createOrFind(name);
    c->setsource(spec);
    
    // get the in-range, which converts to 0-1.
    
    if(tok.getnext()!=T_IN)expected("'in'");
    
    float inmin = tok.getnextfloat();
    if(tok.iserror())expected("float (in input range)");
    if(tok.getnext()!=T_COLON)expected("':' in input range");
    float inmax = tok.getnextfloat();
    if(tok.iserror())expected("float (in input range)");
    c->setinrange(inmin,inmax);
    
}

// parse plugin data: currently just shortnames for ports
void parsePlugin(){
    string name=getnextident();
    PluginData *p = PluginMgr::getPlugin(name);
    if(tok.getnext()!=T_OCURLY)expected("'{'");
    
    if(tok.getnext()==T_NAMES){
        parseList([=]{
                  string sn = getnextident();
                  string ln = getnextidentorstring();
                  p->addShortPortName(sn,ln);
              });
    } else tok.rewind();
    
    if(tok.getnext()!=T_CCURLY)expected("'}'");
    
}

void parseMaster(){
    if(tok.getnext()!=T_GAIN)expected("'gain'");
    parseValue(Bounds(),Process::masterGain)->setname("master gain");
    if(tok.getnext()!=T_PAN)expected("'pan'");
    parseValue(Bounds(),Process::masterPan)->setname("master pan");
}

/// parses the config file as a single string
void parseConfigData(const char *s){
    extern void parseStereoChain();
    extern vector<string> pluginDirs;
    tok.reset(s);
    string ss;
    for(;;){
        switch(tok.getnext()){
        case T_PLUGINDIR:
            ss=getnextstring();
            PluginMgr::loadFilesIn(ss.c_str());
            pluginDirs.push_back(ss);
            break;
        case T_CHANS:
            parseList(parseChan);
            break;
        case T_CTRL:
            parseList(parseCtrl);
            break;
        case T_CHAIN:
            parseList(parseStereoChain);
            break;
        case T_PLUGINS:
            parseList(parsePlugin);
            break;
        case T_MASTER:
            parseMaster();
            break;
        case T_END:
            return;
        default:
            expected("chans, ctrls, fx, plugins");
        }
    }
}

void parseConfig(const char *file){
    // parsing - reads entire file
    try {
        tok.init();
        tok.setname("<in>");
        tok.settokens(tokens);
        tok.setcommentlinesequence("#");
        //        tok.settrace(true);
        FILE *a = fopen(file,"r");
        if(a){
            fseek(a,0L,SEEK_END);
            int n = ftell(a);
            fseek(a,0L,SEEK_SET);
            char *fbuf = (char *)malloc(n+1);
            fread(fbuf,sizeof(char),n,a);
            fbuf[n]=0;
            fclose(a);
            parseConfigData(fbuf);
            free(fbuf);
            
        } else 
            throw _("cannot open config file '%s'",file);
    } catch(string s){
        stringstream ss;
        ss << "at line " << tok.getline() << ": " << s;
        throw ss.str();
    }
}
