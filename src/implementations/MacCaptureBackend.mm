/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation
 * ============================================================================
 *
 *  File:        MacCaptureBackend.mm
 *  Description: macOS screen capture and window enumeration via CoreGraphics
 *
 *  Author:      Phillip H. Blanton
 *  Created:     Oct 23, 2026
 *
 *  Copyright (c) 2026 Phillip H. Blanton
 *
 *  SPDX-License-Identifier: GPLv3
 *
 *  PluckIt is free and open-source software.
 *  See the LICENSE file in the project root for license details.
 *
 *  "If you can see it, then you can pluck it."
 * ============================================================================
 */

#include "../headers/MacCaptureBackend.h"

#include <QGuiApplication>
#include <QPainter>
#include <QPixmap>
#include <QScreen>

// Apple headers after all Qt headers to avoid macro collisions.
#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#include <unistd.h>

std::unique_ptr<CaptureBackend> createCaptureBackend()
{
    return std::make_unique<MacCaptureBackend>();
}

MacCaptureBackend::MacCaptureBackend()
{
    // Trigger the system permission dialog on first run. On macOS 10.15+ all
    // screen capture APIs require the Screen Recording entitlement; without it
    // grabWindow() returns blank/black images. CGRequestScreenCaptureAccess()
    // is a no-op if already granted.
    if (@available(macOS 10.15, *)) {
        if (!CGPreflightScreenCaptureAccess()) {
            CGRequestScreenCaptureAccess();
        }
    }
}

MacCaptureBackend::~MacCaptureBackend() = default;

bool MacCaptureBackend::hasScreenRecordingPermission()
{
    if (@available(macOS 10.15, *)) {
        return CGPreflightScreenCaptureAccess();
    }
    return true;
}

QRect MacCaptureBackend::virtualDesktopGeometry() const
{
    QRect united;
    for (QScreen *screen : QGuiApplication::screens()) {
        united = united.united(screen->geometry());
    }
    return united;
}

QImage MacCaptureBackend::grabRect(const QRect &globalRect, bool includeCursor)
{
    Q_UNUSED(includeCursor)

    const QRect target = globalRect.normalized();
    if (target.isEmpty()) {
        return QImage();
    }

    const QList<QScreen *> screens = QGuiApplication::screens();
    // NOTE: assumes a uniform device pixel ratio across monitors. Mixed-DPI
    // rigs will need per-screen-aware compositing; revisit if that bites us.
    const qreal dpr = screens.isEmpty() ? 1.0 : screens.first()->devicePixelRatio();

    QImage composite(qRound(target.width() * dpr), qRound(target.height() * dpr),
                     QImage::Format_ARGB32_Premultiplied);
    composite.setDevicePixelRatio(dpr);
    composite.fill(Qt::black);

    QPainter painter(&composite);
    for (QScreen *screen : screens) {
        const QRect screenGeom = screen->geometry();
        const QRect intersection = screenGeom.intersected(target);
        if (intersection.isEmpty()) {
            continue;
        }

        const QRect local = intersection.translated(-screenGeom.topLeft());
        QPixmap grabbed = screen->grabWindow(0, local.x(), local.y(),
                                             local.width(), local.height());
        if (grabbed.isNull()) {
            continue;
        }

        const QPoint dest = intersection.topLeft() - target.topLeft();
        painter.drawPixmap(dest, grabbed);
    }

    painter.end();
    return composite;
}

WindowInfo MacCaptureBackend::windowAt(const QPoint &globalPos,
                                       const QVector<WindowHandle> &excluding)
{
    Q_UNUSED(excluding)
    WindowInfo result;

    // CGWindowListCopyWindowInfo returns windows front-to-back (topmost first).
    CFArrayRef windowList = CGWindowListCopyWindowInfo(
        kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
        kCGNullWindowID);
    if (!windowList) {
        return result;
    }

    // Skip every window owned by this process (overlay, floating widget,
    // editor). Using the PID is simpler and more reliable than converting
    // Qt's WId (NSView*) to a CGWindowID.
    const pid_t ourPID = getpid();

    const CFIndex count = CFArrayGetCount(windowList);
    for (CFIndex i = 0; i < count; ++i) {
        auto *info = static_cast<CFDictionaryRef>(
            CFArrayGetValueAtIndex(windowList, i));

        // Skip our own process windows.
        pid_t ownerPID = 0;
        auto *pidRef = static_cast<CFNumberRef>(
            CFDictionaryGetValue(info, kCGWindowOwnerPID));
        if (pidRef) {
            CFNumberGetValue(pidRef, kCFNumberSInt32Type, &ownerPID);
        }
        if (ownerPID == ourPID) {
            continue;
        }

        // Skip desktop/wallpaper (layer < 0) and system UI chrome (layer >= 20:
        // Dock background, menu bar, status bar items). Normal app windows are
        // at layer 0; floating tool palettes and modal panels are 3–8.
        int32_t layer = 0;
        auto *layerRef = static_cast<CFNumberRef>(
            CFDictionaryGetValue(info, kCGWindowLayer));
        if (layerRef) {
            CFNumberGetValue(layerRef, kCFNumberSInt32Type, &layer);
        }
        if (layer < 0 || layer >= 20) {
            continue;
        }

        // Bounds are in global screen coordinates: origin at top-left of the
        // primary display, Y increasing downward -- same as Qt's coordinate
        // system on macOS.
        auto *boundsDict = static_cast<CFDictionaryRef>(
            CFDictionaryGetValue(info, kCGWindowBounds));
        if (!boundsDict) {
            continue;
        }
        CGRect cgBounds = CGRectZero;
        if (!CGRectMakeWithDictionaryRepresentation(boundsDict, &cgBounds)) {
            continue;
        }

        const QRect geom(static_cast<int>(cgBounds.origin.x),
                         static_cast<int>(cgBounds.origin.y),
                         static_cast<int>(cgBounds.size.width),
                         static_cast<int>(cgBounds.size.height));

        if (!geom.contains(globalPos)) {
            continue;
        }

        uint32_t windowID = 0;
        auto *widRef = static_cast<CFNumberRef>(
            CFDictionaryGetValue(info, kCGWindowNumber));
        if (widRef) {
            CFNumberGetValue(widRef, kCFNumberSInt32Type, &windowID);
        }

        // Prefer the window title; fall back to the owning app name.
        QString title;
        auto *nameRef = static_cast<CFStringRef>(
            CFDictionaryGetValue(info, kCGWindowName));
        if (nameRef) {
            title = QString::fromCFString(nameRef);
        }
        if (title.isEmpty()) {
            auto *ownerRef = static_cast<CFStringRef>(
                CFDictionaryGetValue(info, kCGWindowOwnerName));
            if (ownerRef) {
                title = QString::fromCFString(ownerRef);
            }
        }

        result.handle   = static_cast<WindowHandle>(windowID);
        result.geometry = geom;
        result.title    = title;
        break;
    }

    CFRelease(windowList);
    return result;
}
