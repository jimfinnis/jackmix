/**
 * @file help.h
 * @brief Help texts. File included only in monitor.cpp
 *
 */

#ifndef __HELP_H
#define __HELP_H


/// Each help screen is a null-term array of columns, each of
/// which is a null-term array of strings.
/// Control chars:
///  [ start bold
///  ] end bold
///  { start key name
///  } end key name


static const char *MainHelpCol1[] = {
//   0123456789012345678901234567890
    "[         Main mode]",
    "{z,x}      - prev/next channel",
    "{UP/DOWN}  - change gain",
    "{LFT/RGT}  - change pan",
    "{q}        - quit",
    "{c}        - switch to channel",
    "{s}        - save config",
    "           mode",
    NULL
};

static const char *MainHelpCol2[] = {
//   0123456789012345678901234567890
    "[      Channel mode]",
    "{ESC} - return to main mode",
    NULL
};

static const char **MainHelp[]=
{
    MainHelpCol1,MainHelpCol2
};
    

#endif /* __HELP_H */