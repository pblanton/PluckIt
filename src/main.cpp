/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        main.cpp
 *  Description: Application entry point and single-instance guard
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

#include "FloatingWidget.h"

#include <QApplication>
#include <QIcon>
#include <QLocalServer>
#include <QLocalSocket>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setQuitOnLastWindowClosed(false);
    QCoreApplication::setOrganizationName(QStringLiteral("PluckIt"));
    QCoreApplication::setApplicationName(QStringLiteral("PluckIt"));
    QGuiApplication::setDesktopFileName(QStringLiteral("pluckit"));
    QApplication::setWindowIcon(QIcon::fromTheme(QStringLiteral("pluckit")));

    // Single-instance guard: if another PluckIt is already running, just exit.
    const QString key = QStringLiteral("pluckit-instance-%1").arg(qEnvironmentVariable("USER"));
    QLocalSocket probe;
    probe.connectToServer(key);
    if (probe.waitForConnected(100)) {
        return 0;
    }
    QLocalServer::removeServer(key);
    QLocalServer server;
    server.listen(key);

    FloatingWidget widget;
    widget.show();

    return app.exec();
}
