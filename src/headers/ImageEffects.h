/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        ImageEffects.h
 *  Description: Destructive pixelate and blur effects for redaction
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
#include <QRect>

// In-place, destructive pixel effects applied to a sub-region of an image.
// Used for redacting sensitive content (passwords, etc.) before sharing a
// capture, so unlike annotations these aren't kept as movable overlays.
namespace ImageEffects {

void applyPixelate(QImage &image, const QRect &region, int blockSize = 14);
void applyBoxBlur(QImage &image, const QRect &region, int radius = 8);

} // namespace ImageEffects
