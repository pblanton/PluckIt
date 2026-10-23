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
#include <QPointer>
#include <QString>

#include "EditorCanvas.h"

class QAction;
class QActionGroup;
class QFontComboBox;
class QSpinBox;
class QStackedWidget;
class QTabWidget;
class QToolBar;
class QToolButton;

// The annotation editor. There is only ever one: captures open as tabs in the
// existing window rather than spawning a window each, so use acquire() instead
// of constructing one. Each tab holds one image; Save overwrites that tab's
// file in place, and closing a tab without saving leaves the original raw
// capture untouched. The window also hosts the settings page, and can sit on
// that page alone when there is nothing to annotate yet.
class EditorWindow : public QMainWindow {
    Q_OBJECT
public:
    static EditorWindow *instance();
    static EditorWindow &acquire();

    // Adds a tab for the image (or switches to the existing tab for that path),
    // shows the image page, and brings the window to the front.
    void openImage(const QImage &image, const QString &savedPath);
    bool hasImages() const;

    void showSettingsPage();
    void showImagePage();
    void present();

private:
    EditorWindow();

    EditorCanvas *currentCanvas() const;
    QString currentPath() const;
    void applyToolState(EditorCanvas *canvas);
    void onCurrentTabChanged();
    void closeTab(int index);
    void updateTextBar();
    void onTextFormatChanged(const QFont &font, const QColor &color);
    void updateActionStates();
    void updateWindowTitle();
    void pickColor();
    void saveToFile();
    void copyToClipboard();

    static QPointer<EditorWindow> s_instance;

    QTabWidget *m_tabs = nullptr;
    QStackedWidget *m_stack = nullptr;
    QToolBar *m_topBar = nullptr;
    QToolBar *m_leftBar = nullptr;
    QAction *m_applyCropAction = nullptr;
    QAction *m_undoAction = nullptr;
    QAction *m_saveAction = nullptr;
    QAction *m_copyAction = nullptr;
    QAction *m_cropAction = nullptr;
    QAction *m_closeTabAction = nullptr;
    QActionGroup *m_toolGroup = nullptr;
    QSpinBox *m_widthSpin = nullptr;
    QToolBar *m_textBar = nullptr;
    QFontComboBox *m_fontCombo = nullptr;
    QSpinBox *m_fontSizeSpin = nullptr;
    QAction *m_boldAction = nullptr;
    QAction *m_italicAction = nullptr;
    QAction *m_underlineAction = nullptr;
    QToolButton *m_colorButton = nullptr;

    EditorCanvas::Tool m_tool = EditorCanvas::Tool::Arrow;
    QColor m_currentColor = QColor(220, 40, 40);
    int m_strokeWidth = 3;
    QFont m_textFont;
};
