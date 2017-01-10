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
    "{s}        - solo channel",
    "{m}        - mute channel",
    "{ENTER}    - edit channel",
    "{w}        - write config to file",
    "",
    "[        Channel edit mode]",
    "{ENTER}    - return to main mode",
    "{UP/DOWN}  - switch parameter",
    "{LFT/RGT}  - change parameter",
    "{s}        - solo channel",
    "{m}        - mute channel",
    "{DEL}      - remove send",
    "{p}        - toggle send pre/post",
    "{a}        - add send",
    NULL
};

static const char *MainHelpCol2[] = {
//   0123456789012345678901234567890
    "[        Chain mode]",
    "{ENTER}    - return to main mode",
    "{TAB}      - list->chain->effect",
    "[     Chain mode: list]",
    "{UP/DOWN}  - select chain",
    "[     Chain mode: chain]",
    "{UP/DOWN}  - select effect",
    "[     Chain mode: effect]",
    "{UP/DOWN}  - select parameter",
    "{LFT/RGT}  - change parameter",
    
    NULL
};

static const char **MainHelp[]=
{
    MainHelpCol1,MainHelpCol2
};
    

#endif /* __HELP_H */
