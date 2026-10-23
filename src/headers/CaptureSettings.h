/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        CaptureSettings.h
 *  Description: Shared capture options (clipboard, cursor, delay, mode) backed by QSettings
 *
 *  Author:      Phillip H. Blanton
 *  Created:     Oct 19, 2026
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

#include <QObject>

// Single source of truth for the options exposed in the widget's slide-out
// tray and in the editor's settings page. Both UIs observe changed() so they
// stay in sync when either one is edited.
class CaptureSettings : public QObject {
    Q_OBJECT
public:
    enum class Mode { Region, Window, Monitor, AllDisplays };

    static CaptureSettings &instance();

    bool previewInEditor() const;
    bool copyToClipboard() const;
    bool includeCursor() const;
    bool autoHide() const;
    bool hideWhileCapturing() const;
    bool delayEnabled() const;
    int delaySeconds() const;
    Mode mode() const;

    void setPreviewInEditor(bool on);
    void setCopyToClipboard(bool on);
    void setIncludeCursor(bool on);
    void setAutoHide(bool on);
    void setHideWhileCapturing(bool on);
    void setDelayEnabled(bool on);
    void setDelaySeconds(int seconds);
    void setMode(Mode mode);

signals:
    void changed();

private:
    CaptureSettings() = default;
};
