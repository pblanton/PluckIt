/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        X11CaptureBackend.cpp
 *  Description: X11 screen grabbing and window enumeration via Xlib
 *
 *  Author:      Phillip H. Blanton
 *  Created:     Oct 18, 2026
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

#include "X11CaptureBackend.h"

#include <QGuiApplication>
#include <QPainter>
#include <QPixmap>
#include <QScreen>

// Included last and deliberately after all Qt headers above -- see the
// comment in X11CaptureBackend.h for why.
#include <X11/Xatom.h>
#include <X11/Xlib.h>

std::unique_ptr<CaptureBackend> createCaptureBackend()
{
    // Only an X11 backend exists today; a Wayland backend would be
    // selected here based on qgetenv("XDG_SESSION_TYPE").
    return std::make_unique<X11CaptureBackend>();
}

X11CaptureBackend::X11CaptureBackend()
{
    m_display = XOpenDisplay(nullptr);
    if (!m_display) {
        qFatal("PluckIt: unable to open X11 display");
    }
    m_root = DefaultRootWindow(m_display);
    m_clientListStacking = XInternAtom(m_display, "_NET_CLIENT_LIST_STACKING", False);
    m_wmName = XInternAtom(m_display, "_NET_WM_NAME", False);
    m_utf8String = XInternAtom(m_display, "UTF8_STRING", False);
    m_frameExtents = XInternAtom(m_display, "_NET_FRAME_EXTENTS", False);
}

X11CaptureBackend::~X11CaptureBackend()
{
    if (m_display) {
        XCloseDisplay(m_display);
    }
}

QRect X11CaptureBackend::virtualDesktopGeometry() const
{
    QRect united;
    for (QScreen *screen : QGuiApplication::screens()) {
        united = united.united(screen->geometry());
    }
    return united;
}

QImage X11CaptureBackend::grabRect(const QRect &globalRect)
{
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
        QPixmap grabbed = screen->grabWindow(0, local.x(), local.y(), local.width(), local.height());
        if (grabbed.isNull()) {
            continue;
        }

        const QPoint dest = intersection.topLeft() - target.topLeft();
        painter.drawPixmap(dest, grabbed);
    }
    painter.end();

    return composite;
}

bool X11CaptureBackend::isViewable(Window w) const
{
    XWindowAttributes attrs;
    if (!XGetWindowAttributes(m_display, w, &attrs)) {
        return false;
    }
    return attrs.map_state == IsViewable;
}

QRect X11CaptureBackend::windowGeometry(Window w) const
{
    XWindowAttributes attrs;
    if (!XGetWindowAttributes(m_display, w, &attrs)) {
        return QRect();
    }

    Window child;
    int rootX = 0;
    int rootY = 0;
    XTranslateCoordinates(m_display, w, m_root, 0, 0, &rootX, &rootY, &child);

    QRect geom(rootX, rootY, attrs.width, attrs.height);

    Atom actualType;
    int actualFormat;
    unsigned long nItems = 0;
    unsigned long bytesAfter = 0;
    unsigned char *data = nullptr;
    if (XGetWindowProperty(m_display, w, m_frameExtents, 0, 4, False, XA_CARDINAL,
                            &actualType, &actualFormat, &nItems, &bytesAfter, &data) == Success
        && data && nItems == 4) {
        long *extents = reinterpret_cast<long *>(data);
        geom.adjust(-extents[0], -extents[2], extents[1], extents[3]);
    }
    if (data) {
        XFree(data);
    }

    return geom;
}

QString X11CaptureBackend::windowTitle(Window w) const
{
    Atom actualType;
    int actualFormat;
    unsigned long nItems = 0;
    unsigned long bytesAfter = 0;
    unsigned char *data = nullptr;

    if (XGetWindowProperty(m_display, w, m_wmName, 0, 1024, False, m_utf8String,
                            &actualType, &actualFormat, &nItems, &bytesAfter, &data) == Success
        && data) {
        QString title = QString::fromUtf8(reinterpret_cast<char *>(data), int(nItems));
        XFree(data);
        if (!title.isEmpty()) {
            return title;
        }
    }

    char *name = nullptr;
    if (XFetchName(m_display, w, &name) && name) {
        QString title = QString::fromLocal8Bit(name);
        XFree(name);
        return title;
    }

    return QString();
}

WindowInfo X11CaptureBackend::windowAt(const QPoint &globalPos, const QVector<WindowHandle> &excluding)
{
    Atom actualType;
    int actualFormat;
    unsigned long nItems = 0;
    unsigned long bytesAfter = 0;
    unsigned char *data = nullptr;

    WindowInfo result;

    const int status = XGetWindowProperty(m_display, m_root, m_clientListStacking, 0, (~0L), False,
                                           XA_WINDOW, &actualType, &actualFormat, &nItems, &bytesAfter,
                                           &data);
    if (status != Success || !data) {
        if (data) {
            XFree(data);
        }
        return result;
    }

    Window *windows = reinterpret_cast<Window *>(data);
    // _NET_CLIENT_LIST_STACKING is bottom-to-top; walk it in reverse to
    // check the topmost windows first.
    for (long i = long(nItems) - 1; i >= 0; --i) {
        const Window w = windows[i];
        if (excluding.contains(WindowHandle(w))) {
            continue;
        }
        if (!isViewable(w)) {
            continue;
        }
        const QRect geom = windowGeometry(w);
        if (geom.contains(globalPos)) {
            result.handle = WindowHandle(w);
            result.geometry = geom;
            result.title = windowTitle(w);
            break;
        }
    }

    XFree(data);
    return result;
}
