/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        FloatingWidget.h
 *  Description: Always-on-top floating pill and capture-mode panel
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

#include "CaptureBackend.h"

#include <QImage>
#include <QPoint>
#include <QPointer>
#include <QString>
#include <QWidget>
#include <memory>

class QLabel;
class QPushButton;
class QFrame;
class EditorWindow;

// Always-on-top floating control. Collapsed it's a small draggable pill;
// clicking it (without dragging) expands a panel with the four capture
// modes. Right-click for Show Editor / Configure / Exit. This is the whole
// UI for the capture-only MVP.
class FloatingWidget : public QWidget {
    Q_OBJECT
public:
    explicit FloatingWidget(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void toggleExpanded();
    void startRegionCapture();
    void startWindowCapture();
    void showMonitorMenu();
    void captureAllDisplays();
    void onRegionSelected(const QRect &globalRect);
    void onWindowSelected(const WindowInfo &info);
    void onSelectionCancelled();
    void finishCapture(const QImage &image);
    void flashStatus(const QString &text);

    void restorePosition();
    void savePosition();
    void loadLastCaptureFromDisk();
    void openEditorOn(const QImage &image, const QString &path);
    void showLastCaptureInEditor();
    void showConfigure();

    std::unique_ptr<CaptureBackend> m_backend;
    QPushButton *m_pillButton = nullptr;
    QFrame *m_panel = nullptr;
    QLabel *m_statusLabel = nullptr;

    QPoint m_dragStartGlobalPos;
    QPoint m_dragStartWidgetPos;
    bool m_dragging = false;

    QImage m_lastCaptureImage;
    QString m_lastCapturePath;
    QPointer<EditorWindow> m_lastEditor;
};
