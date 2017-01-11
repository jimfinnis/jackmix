/**
 * @file editor.h
 * @brief Superclass for "editors" - string list selector, line
 * editor etc.
 *
 */

#ifndef __EDITOR_H
#define __EDITOR_H

// state
enum EditState {
    Idle,Running,Aborted,Finished
};

class Editor {
protected:
    std::string prompt;
    EditState state = Idle;
    
    void begin(std::string p){
        state = Running;
        prompt = p;
    }
public:    
    EditState getState(){return state;}
};
    

    
          



#endif /* __EDITOR_H */
