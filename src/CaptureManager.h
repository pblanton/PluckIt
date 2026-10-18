/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        CaptureManager.h
 *  Description: Saving of captured images to the Captures folder
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

#pragma once

#include <QImage>
#include <QString>

namespace CaptureManager {

// Saves the image as a timestamped PNG under ~/Pictures/Captures (created if
// missing). Returns the full path written, or an empty string on failure.
QString saveCapture(const QImage &image);

} // namespace CaptureManager
