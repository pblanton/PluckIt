#include "../headers/MacWindowUtils.h"

#include <QWidget>

#import <AppKit/AppKit.h>

void mac_setHidesOnDeactivate(QWidget *widget, bool hide)
{
    if (!widget) {
        return;
    }
    NSView *view = (__bridge NSView *)reinterpret_cast<void *>(widget->winId());
    NSWindow *nsw = [view window];
    if (nsw) {
        [nsw setHidesOnDeactivate:(hide ? YES : NO)];
    }
}
