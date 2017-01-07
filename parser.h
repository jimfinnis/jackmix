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
inline void expected(std::string s){
    std::stringstream ss;
    ss << "Expected " << s << ", got '" << tok.getstring() << "'";
    throw(ss.str());
}

inline std::string getnextident(){
    if(tok.getnext()!=T_IDENT)
        expected("identifier");
    return std::string(tok.getstring());
}

inline float getnextfloat(){
    float f = tok.getnextfloat();
    if(tok.iserror())expected("'number'");
    return f;
}

inline std::string getnextidentorstring(){
    int t = tok.getnext();
    if(t!=T_IDENT && t!=T_STRING)
        expected("identifier or string");
    return std::string(tok.getstring());
}

inline std::string getnextstring(){
    int t = tok.getnext();
    if(t!=T_STRING)
        expected("string");
    return std::string(tok.getstring());
}

// parses { x,x,x,..x } where x is a parser function.
inline void parseList(std::function<void()> f){
    if(tok.getnext()!=T_OCURLY)
        expected("'{'");
    if(tok.getnext()==T_CCURLY) // empty list?
        return;
    tok.rewind(); // wasn't.
        
    for(;;){
        f();
        int t = tok.getnext();
        if(t==T_CCURLY)break;
        else if(t!=T_COMMA)expected("',' or '{'");
    }
}    

#endif /* __PARSER_H */
