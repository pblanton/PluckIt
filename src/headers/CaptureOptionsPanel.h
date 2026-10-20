/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        CaptureOptionsPanel.h
 *  Description: Option rows shared by the slide-out tray and the editor settings page
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

#include <QWidget>

class QCheckBox;
class QSpinBox;
class ToggleSwitch;

// Open Capture in Editor / Copy to Clipboard / Include Cursor / Capture Delay.
// Writes straight through to CaptureSettings and refreshes itself when the
// other instance (tray vs. settings page) changes a value. Colors are left to
// the parent's palette/stylesheet so it fits both the dark tray and the
// regular editor window.
class CaptureOptionsPanel : public QWidget {
    Q_OBJECT
public:
    explicit CaptureOptionsPanel(QWidget *parent = nullptr);

private:
    void refresh();

    ToggleSwitch *m_preview = nullptr;
    ToggleSwitch *m_clipboard = nullptr;
    ToggleSwitch *m_cursor = nullptr;
    ToggleSwitch *m_autoHide = nullptr;
    QCheckBox *m_delayCheck = nullptr;
    QSpinBox *m_delaySpin = nullptr;
};
