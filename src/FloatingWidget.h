/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        FloatingWidget.h
 *  Description: Dockable capture widget declaration
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
#include "CaptureTray.h"

#include <QElapsedTimer>
#include <QImage>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QString>
#include <QWidget>
#include <memory>

class CaptureButton;
class EditorWindow;
class QGridLayout;
class QPushButton;
class QScreen;
class QTimer;
class QVariantAnimation;

// Always-on-top capture widget docked to a screen edge. Dragging it toward a
// different edge (or monitor) re-docks it and rearranges the buttons between
// a horizontal strip (top/bottom) and a vertical one (left/right). The big
// camera button captures with the current mode, the gear opens the editor's
// settings page, the pencil reopens the last capture in the editor, and the
// hamburger slides out a tray with the capture options. A thin strip with a
// red grip line always runs along the docked edge; when idle everything else
// collapses into that strip, and it slides back out after the pointer has
// dwelled on it.
class FloatingWidget : public QWidget {
    Q_OBJECT
public:
    explicit FloatingWidget(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void buildUi();
    void applyLayout();
    void dockAt(DockEdge edge, QScreen *screen, int alongCenter);
    DockEdge pickEdge(const QPoint &globalPos, QScreen *screen) const;
    void restoreDock();
    void saveDock();

    void applySlide(qreal t);
    void pollHover();
    void expand();
    void collapse();

    void dragPress(const QPoint &globalPos);
    bool dragMove(const QPoint &globalPos);
    bool dragRelease();

    void toggleTray();
    void closeTray();

    void onCaptureClicked();
    void showModeMenu();
    bool chooseMonitor();
    void startCountdown();
    void cancelCountdown();
    void tickCountdown();
    void beginCapture();
    void runCapture();
    void onRegionSelected(const QRect &globalRect);
    void onWindowSelected(const WindowInfo &info);
    void onSelectionCancelled();
    void completeCapture(const QRect &globalRect);
    void finishCapture(const QImage &image);
    void flashStatus(const QString &text);

    void loadLastCaptureFromDisk();
    void openEditorOn(const QImage &image, const QString &path);
    void showLastCaptureInEditor();
    void showSettings();

    std::unique_ptr<CaptureBackend> m_backend;
    QWidget *m_viewport = nullptr;
    QWidget *m_body = nullptr;
    QGridLayout *m_grid = nullptr;
    QPushButton *m_gearButton = nullptr;
    QPushButton *m_editButton = nullptr;
    QPushButton *m_menuButton = nullptr;
    CaptureButton *m_captureButton = nullptr;
    CaptureTray *m_tray = nullptr;
    QTimer *m_countdownTimer = nullptr;
    int m_countdownRemaining = 0;
    QRect m_pendingMonitor;

    QVariantAnimation *m_slide = nullptr;
    QTimer *m_hoverTimer = nullptr;
    QElapsedTimer m_dwell;
    QRect m_fullRect;
    qreal m_slideT = 1.0;
    bool m_collapsed = false;

    DockEdge m_edge = DockEdge::Right;
    QPointer<QScreen> m_screen;
    int m_along = 0;

    QPoint m_pressGlobalPos;
    int m_grabOffset = 0;
    bool m_dragging = false;

    QImage m_lastCaptureImage;
    QString m_lastCapturePath;
    QPointer<EditorWindow> m_lastEditor;
};
