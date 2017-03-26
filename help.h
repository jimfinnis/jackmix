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
    "{UP/DOWN}  - nudge gain",
    "{LFT/RGT}  - nudge pan",
    "{DEL}      - delete channel",
    "{q}        - quit",
    "{s}        - solo channel",
    "{m}        - mute channel",
    "{g}        - set gain",
    "{p}        - set pan",
    "{c}        - chain editor",
    "{C}        - controller editor",
    "{ENTER}    - edit channel",
    "{w}        - write config to file",
    "",
    "[        Channel edit mode]",
    "{ENTER}    - return to main mode",
    "{UP/DOWN}  - switch parameter",
    "{LFT/RGT}  - nudge parameter",
    "{g}        - set gain",
    "{p}        - set pan",
    "{v}        - set value of parameter",
    "{s}        - solo channel",
    "{m}        - mute channel",
    "{DEL}      - delete send",
    "{t}        - toggle send pre/post",
    "{a}        - add send",
    NULL
};

static const char *MainHelpCol2[] = {
//   0123456789012345678901234567890
    "[        Chain mode]",
    "{ENTER/q}  - return to main mode",
    "{TAB}      - list->chain->effect",
    "{a}        - add chain",
    "{e}        - add effect",
    "{i}        - remap effect input",
    "[     Chain mode: list]",
    "{UP/DOWN}  - select chain",
    "{DEL}      - remove chain",
    "[     Chain mode: chain]",
    "{UP/DOWN}  - select effect",
    "{DEL}      - remove effect",
    "[     Chain mode: effect]",
    "{UP/DOWN}  - select parameter",
    "{LFT/RGT}  - change parameter",
    "",
    "[    Controller edit mode]",
    "{ENTER/q}  - return to main mode",
    "{TAB}      - ctrl list/value list",
    "{UP}       - previous ctrl/value",
    "{DOWN}     - next ctrl/value",
    "{DEL}      - delete ctrl/val. link",
    "{r}        - set/reset input range",
    "{a}        - add new controller",
    
    NULL
};

static const char **MainHelp[]=
{
    MainHelpCol1,MainHelpCol2,NULL
};
    

#endif /* __HELP_H */
