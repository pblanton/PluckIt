/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        CaptureManager.cpp
 *  Description: Saves captured images as timestamped PNGs under ~/Pictures/Captures
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

#include "../headers/CaptureManager.h"

#include <QDateTime>
#include <QDir>
#include <QStandardPaths>

namespace CaptureManager {

QString saveCapture(const QImage &image)
{
    if (image.isNull()) {
        return QString();
    }

    const QString picturesDir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    const QDir capturesDir(picturesDir + QStringLiteral("/Captures"));
    if (!capturesDir.exists() && !capturesDir.mkpath(QStringLiteral("."))) {
        return QString();
    }

    const QString filename = QStringLiteral("capture_%1.png")
                                  .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HHmmss")));
    const QString fullPath = capturesDir.filePath(filename);

    if (!image.save(fullPath, "PNG")) {
        return QString();
    }
    return fullPath;
}

} // namespace CaptureManager
