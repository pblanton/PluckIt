/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        EditorWindow.h
 *  Description: Standalone annotation editor window
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

#include <QColor>
#include <QMainWindow>
#include <QString>

class EditorCanvas;
class QAction;
class QToolButton;

// Standalone annotation editor window opened on a just-captured (and
// already silently-saved) image. Save overwrites that same file in place;
// closing without saving leaves the original raw capture untouched.
class EditorWindow : public QMainWindow {
    Q_OBJECT
public:
    EditorWindow(const QImage &image, const QString &savedPath, QWidget *parent = nullptr);

private:
    void updateActionStates();
    void pickColor();
    void saveToFile();

    EditorCanvas *m_canvas = nullptr;
    QAction *m_applyCropAction = nullptr;
    QAction *m_undoAction = nullptr;
    QToolButton *m_colorButton = nullptr;
    QColor m_currentColor = QColor(220, 40, 40);
    QString m_savedPath;
};
