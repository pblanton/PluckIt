/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        EditorWindow.h
 *  Description: Annotation editor window, including the settings page
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
class QStackedWidget;
class QToolBar;
class QToolButton;

// Standalone annotation editor window opened on a just-captured (and
// already silently-saved) image. Save overwrites that same file in place;
// closing without saving leaves the original raw capture untouched. It also
// hosts the settings page; with a null image it opens on that page alone
// (there's nothing to annotate yet).
class EditorWindow : public QMainWindow {
    Q_OBJECT
public:
    EditorWindow(const QImage &image, const QString &savedPath, QWidget *parent = nullptr);

    void showSettingsPage();
    void showImagePage();

private:
    void updateActionStates();
    void pickColor();
    void saveToFile();
    void copyToClipboard();

    QString imageTitle() const;

    EditorCanvas *m_canvas = nullptr;
    QStackedWidget *m_stack = nullptr;
    QToolBar *m_topBar = nullptr;
    QToolBar *m_leftBar = nullptr;
    bool m_hasImage = false;
    QAction *m_applyCropAction = nullptr;
    QAction *m_undoAction = nullptr;
    QToolButton *m_colorButton = nullptr;
    QColor m_currentColor = QColor(220, 40, 40);
    QString m_savedPath;
};
