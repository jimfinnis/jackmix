/**
 * @file stringsplit.h
 * @brief  Brief description of file.
 *
 */

#ifndef __STRINGSPLIT_H
#define __STRINGSPLIT_H

#include <string>
#include <sstream>
#include <vector>

inline void split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
}


inline std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

#endif /* __STRINGSPLIT_H */
