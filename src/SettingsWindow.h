/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        SettingsWindow.h
 *  Description: Application settings dialog
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

#include <QDialog>

class QCheckBox;

// Minimal settings dialog, backed directly by QSettings (each control writes
// through immediately on change -- there's no separate Save step). Deliberately
// parented to the most recently opened EditorWindow when one exists, since
// anchoring a dialog to the tiny floating pill looks wrong.
class SettingsWindow : public QDialog {
    Q_OBJECT
public:
    explicit SettingsWindow(QWidget *parent = nullptr);

private:
    QCheckBox *m_openEditorCheck = nullptr;
};
