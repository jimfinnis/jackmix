/**
 * @file screenhelp.h
 * @brief  Brief description of file.
 *
 */

#ifndef __SCREENHELP_H
#define __SCREENHELP_H

extern class HelpScreen : public Screen {
public:
    virtual void display(struct MonitorData *d);
    virtual void flow(class InputManager *im);
} scrHelp;


#endif /* __SCREENHELP_H */
