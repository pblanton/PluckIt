#pragma once

#ifdef Q_OS_MAC

class QWidget;

// Prevents the widget's NSWindow from auto-hiding when the application
// loses focus (the default macOS behavior for Qt::Tool windows).
void mac_setHidesOnDeactivate(QWidget *widget, bool hide);

#endif
