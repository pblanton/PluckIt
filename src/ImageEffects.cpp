/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        ImageEffects.cpp
 *  Description: Pixelate and box-blur implementations for image regions
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

#include "ImageEffects.h"

#include <QPainter>

namespace ImageEffects {

void applyPixelate(QImage &image, const QRect &region, int blockSize)
{
    const QRect r = region.normalized().intersected(image.rect());
    if (r.isEmpty() || blockSize < 1) {
        return;
    }

    QPainter painter(&image);
    for (int by = r.top(); by <= r.bottom(); by += blockSize) {
        for (int bx = r.left(); bx <= r.right(); bx += blockSize) {
            const QRect block = QRect(bx, by, blockSize, blockSize).intersected(r);
            if (block.isEmpty()) {
                continue;
            }

            long sumR = 0, sumG = 0, sumB = 0;
            int count = 0;
            for (int y = block.top(); y <= block.bottom(); ++y) {
                for (int x = block.left(); x <= block.right(); ++x) {
                    const QColor c = image.pixelColor(x, y);
                    sumR += c.red();
                    sumG += c.green();
                    sumB += c.blue();
                    ++count;
                }
            }
            if (count == 0) {
                continue;
            }
            painter.fillRect(block, QColor(int(sumR / count), int(sumG / count), int(sumB / count)));
        }
    }
}

// Separable box blur, operating only on pixels within the region itself
// (edge-clamped at the region boundary). Good enough to make text
// unreadable for redaction purposes without needing a full image copy.
void applyBoxBlur(QImage &image, const QRect &region, int radius)
{
    const QRect r = region.normalized().intersected(image.rect());
    if (r.isEmpty() || radius < 1) {
        return;
    }

    QImage src = image.copy(r).convertToFormat(QImage::Format_ARGB32);
    const int w = src.width();
    const int h = src.height();
    QImage pass(w, h, QImage::Format_ARGB32);

    // horizontal pass: src -> pass
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            long sumR = 0, sumG = 0, sumB = 0, sumA = 0;
            int count = 0;
            for (int dx = -radius; dx <= radius; ++dx) {
                const int sx = qBound(0, x + dx, w - 1);
                const QRgb px = src.pixel(sx, y);
                sumR += qRed(px);
                sumG += qGreen(px);
                sumB += qBlue(px);
                sumA += qAlpha(px);
                ++count;
            }
            pass.setPixel(x, y, qRgba(int(sumR / count), int(sumG / count), int(sumB / count),
                                       int(sumA / count)));
        }
    }

    // vertical pass: pass -> src
    for (int x = 0; x < w; ++x) {
        for (int y = 0; y < h; ++y) {
            long sumR = 0, sumG = 0, sumB = 0, sumA = 0;
            int count = 0;
            for (int dy = -radius; dy <= radius; ++dy) {
                const int sy = qBound(0, y + dy, h - 1);
                const QRgb px = pass.pixel(x, sy);
                sumR += qRed(px);
                sumG += qGreen(px);
                sumB += qBlue(px);
                sumA += qAlpha(px);
                ++count;
            }
            src.setPixel(x, y, qRgba(int(sumR / count), int(sumG / count), int(sumB / count),
                                      int(sumA / count)));
        }
    }

    QPainter painter(&image);
    painter.drawImage(r.topLeft(), src);
}

} // namespace ImageEffects
