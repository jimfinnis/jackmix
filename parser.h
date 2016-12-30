/**
 * @file parser.h
 * @brief Parser helper stuff
 *
 */

#ifndef __PARSER_H
#define __PARSER_H

#include <sstream>
#include <functional>

extern Tokeniser tok;
inline void expected(string s){
    stringstream ss;
    ss << "Expected " << s << ", got '" << tok.getstring() << "'";
    throw(ss.str());
}

inline string getnextident(){
    if(tok.getnext()!=T_IDENT)
        expected("identifier");
    return string(tok.getstring());
}

inline float getnextfloat(){
    float f = tok.getnextfloat();
    if(tok.iserror())expected("'number'");
    return f;
}

inline string getnextidentorstring(){
    int t = tok.getnext();
    if(t!=T_IDENT && t!=T_STRING)
        expected("identifier or string");
    return string(tok.getstring());
}

// parses { x,x,x,..x } where x is a parser function.
inline void parseList(function<void()> f){
    if(tok.getnext()!=T_OCURLY)
        expected("'{'");
    for(;;){
        f();
        int t = tok.getnext();
        if(t==T_CCURLY)break;
        else if(t!=T_COMMA)expected("',' or '{'");
    }
}    

#endif /* __PARSER_H */
