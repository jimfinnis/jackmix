/**
 * @file save.h
 * @brief  Brief description of file.
 *
 */

#ifndef __SAVE_H
#define __SAVE_H

#include <vector>
#include <string>

/// save the system configuration to a file
void saveConfig(const char *fn);

/// produce a string which is a separation of all the strings
/// in the vector by "delim".
std::string intercalate(std::vector<std::string> v,const char *delim);


#endif /* __SAVE_H */
