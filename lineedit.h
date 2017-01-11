/**
 * @file lineedit.h
 * @brief  Brief description of file.
 *
 */

#ifndef __LINEEDIT_H
#define __LINEEDIT_H

#include <string>

#include "editor.h"

class LineEdit : public Editor {
    std::string data;
    unsigned int cursor;
public:
    void begin(std::string p){
        Editor::begin(p);
        data = "";
        cursor=0;
    }
    
    // call when a thing has noticed that we're done.
    // will return data and reset state.
    std::string consume(){
        state = Idle;
        return data;
    }
    void display(int y,int x);
    
    EditState handleKey(int k);
};
          
    

#endif /* __LINEEDIT_H */
