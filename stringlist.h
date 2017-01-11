/**
 * @file stringlist.h
 * @brief Something for choosing from a list of strings, works
 * a bit like LineEdit.
 *
 */

#ifndef __STRINGLIST_H
#define __STRINGLIST_H

#include <string>
#include <vector>

#include "editor.h"

class StringList : public Editor {
    // used to filter the list
    // for only items starting with prefix.
    std::string prefix;
    std::vector<std::string> list; // the input list
    
    // the input list after prefix filter applied
    std::vector<std::string> listFiltered;
    unsigned int cursor; // currently selected item
    unsigned int pagelen;
    
    void recalcFilter();
public:
    void begin(std::string p,std::vector<std::string>& l){
        Editor::begin(p);
        cursor = 0;
        list = l;
        listFiltered = list;
        prefix = "";
    }
    
    // call when a thing has noticed that we're done.
    // will return data and reset state.
    std::string consume(){
        state = Idle;
        if(listFiltered.size() && cursor<listFiltered.size())
            return listFiltered[cursor];
        else
            return "";
    }
    void display();
    
    EditState handleKey(int k);
};



#endif /* __STRINGLIST_H */
