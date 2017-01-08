/**
 * @file lineedit.h
 * @brief  Brief description of file.
 *
 */

#ifndef __LINEEDIT_H
#define __LINEEDIT_H

#include <string>

enum LineEditState {
    Idle,Running,Aborted,Finished
};
          

class LineEdit {
    std::string prompt;
    std::string data;
    unsigned int cursor;
    LineEditState state = Idle;
public:
    void begin(std::string p){
        prompt = p;
        data = "";
        state=Running;
        cursor=0;
    }
    
    // call when a thing has noticed that its string read is
    // Finished - will return data and reset state.
    std::string consume(){
        state = Idle;
        return data;
    }
    
    LineEditState getState(){return state;}
    void display(int y,int x);
    
    LineEditState handleKey(int k);
};
          
    

#endif /* __LINEEDIT_H */
