/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        FloatingWidget.cpp
 *  Description: Floating pill UI, capture-mode launching, and context menu
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

#include "FloatingWidget.h"

#include "CaptureManager.h"
#include "EditorWindow.h"
#include "RegionSelectOverlay.h"
#include "SettingsKeys.h"
#include "SettingsWindow.h"
#include "WindowSelectOverlay.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QFrame>
#include <QGuiApplication>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QScreen>
#include <QSettings>
#include <QTimer>
#include <QVBoxLayout>

FloatingWidget::FloatingWidget(QWidget *parent)
    : QWidget(parent)
    , m_backend(createCaptureBackend())
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    m_pillButton = new QPushButton(QStringLiteral("PluckIt"), this);
    m_pillButton->setFixedSize(76, 36);
    m_pillButton->setStyleSheet(
        "QPushButton { background-color: #2d2d2d; color: white; border-radius: 18px; "
        "font-weight: bold; } QPushButton:hover { background-color: #3d3d3d; }");
    layout->addWidget(m_pillButton);

    m_panel = new QFrame(this);
    m_panel->setStyleSheet("QFrame { background-color: #2d2d2d; border-radius: 10px; }"
                            "QPushButton { background-color: #3d3d3d; color: white; border: none; "
                            "padding: 6px; border-radius: 6px; } QPushButton:hover { background-color: #4d4d4d; }");
    auto *panelLayout = new QVBoxLayout(m_panel);
    panelLayout->setSpacing(4);

    auto *regionButton = new QPushButton(QStringLiteral("Region"), m_panel);
    auto *windowButton = new QPushButton(QStringLiteral("Window"), m_panel);
    auto *monitorButton = new QPushButton(QStringLiteral("Monitor"), m_panel);
    auto *allDisplaysButton = new QPushButton(QStringLiteral("All Displays"), m_panel);
    m_statusLabel = new QLabel(m_panel);
    m_statusLabel->setStyleSheet("QLabel { color: #8fd17a; }");
    m_statusLabel->setWordWrap(true);

    panelLayout->addWidget(regionButton);
    panelLayout->addWidget(windowButton);
    panelLayout->addWidget(monitorButton);
    panelLayout->addWidget(allDisplaysButton);
    panelLayout->addWidget(m_statusLabel);

    layout->addWidget(m_panel);
    m_panel->setVisible(false);

    connect(m_pillButton, &QPushButton::clicked, this, &FloatingWidget::toggleExpanded);
    connect(regionButton, &QPushButton::clicked, this, &FloatingWidget::startRegionCapture);
    connect(windowButton, &QPushButton::clicked, this, &FloatingWidget::startWindowCapture);
    connect(monitorButton, &QPushButton::clicked, this, &FloatingWidget::showMonitorMenu);
    connect(allDisplaysButton, &QPushButton::clicked, this, &FloatingWidget::captureAllDisplays);

    adjustSize();
    restorePosition();
    loadLastCaptureFromDisk();
}

void FloatingWidget::toggleExpanded()
{
    m_panel->setVisible(!m_panel->isVisible());
    adjustSize();
}

void FloatingWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartGlobalPos = event->globalPosition().toPoint();
        m_dragStartWidgetPos = pos();
        m_dragging = false;
    }
    QWidget::mousePressEvent(event);
}

void FloatingWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        const QPoint delta = event->globalPosition().toPoint() - m_dragStartGlobalPos;
        if (m_dragging || delta.manhattanLength() > 4) {
            m_dragging = true;
            move(m_dragStartWidgetPos + delta);
        }
    }
    QWidget::mouseMoveEvent(event);
}

void FloatingWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_dragging) {
        savePosition();
    }
    m_dragging = false;
    QWidget::mouseReleaseEvent(event);
}

void FloatingWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    QAction *showEditorAction = menu.addAction(QStringLiteral("Show Editor"));
    showEditorAction->setEnabled(!m_lastCapturePath.isEmpty());
    QAction *configureAction = menu.addAction(QStringLiteral("Configure"));
    menu.addSeparator();
    QAction *exitAction = menu.addAction(QStringLiteral("Exit"));

    QAction *chosen = menu.exec(event->globalPos());
    if (chosen == showEditorAction) {
        showLastCaptureInEditor();
    } else if (chosen == configureAction) {
        showConfigure();
    } else if (chosen == exitAction) {
        QApplication::quit();
    }
}

void FloatingWidget::startRegionCapture()
{
    hide();
    auto *overlay = new RegionSelectOverlay(m_backend->virtualDesktopGeometry());
    connect(overlay, &RegionSelectOverlay::regionSelected, this, &FloatingWidget::onRegionSelected);
    connect(overlay, &RegionSelectOverlay::cancelled, this, &FloatingWidget::onSelectionCancelled);
    overlay->show();
}

void FloatingWidget::startWindowCapture()
{
    hide();
    auto *overlay = new WindowSelectOverlay(m_backend.get(), m_backend->virtualDesktopGeometry());
    connect(overlay, &WindowSelectOverlay::windowSelected, this, &FloatingWidget::onWindowSelected);
    connect(overlay, &WindowSelectOverlay::cancelled, this, &FloatingWidget::onSelectionCancelled);
    overlay->show();
}

void FloatingWidget::showMonitorMenu()
{
    auto *menu = new QMenu(this);
    const QList<QScreen *> screens = QGuiApplication::screens();
    for (QScreen *screen : screens) {
        const QRect g = screen->geometry();
        QAction *action = menu->addAction(QStringLiteral("%1 (%2x%3)")
                                               .arg(screen->name())
                                               .arg(g.width())
                                               .arg(g.height()));
        connect(action, &QAction::triggered, this, [this, g]() {
            finishCapture(m_backend->grabRect(g));
        });
    }
    menu->exec(m_pillButton->mapToGlobal(QPoint(0, m_pillButton->height())));
    menu->deleteLater();
}

void FloatingWidget::captureAllDisplays()
{
    finishCapture(m_backend->grabRect(m_backend->virtualDesktopGeometry()));
}

void FloatingWidget::onRegionSelected(const QRect &globalRect)
{
    finishCapture(m_backend->grabRect(globalRect));
    show();
}

void FloatingWidget::onWindowSelected(const WindowInfo &info)
{
    finishCapture(m_backend->grabRect(info.geometry));
    show();
}

void FloatingWidget::onSelectionCancelled()
{
    show();
}

void FloatingWidget::finishCapture(const QImage &image)
{
    const QString path = CaptureManager::saveCapture(image);
    if (path.isEmpty()) {
        flashStatus(QStringLiteral("Capture failed"));
        return;
    }

    flashStatus(QStringLiteral("Saved %1").arg(QFileInfo(path).fileName()));

    m_lastCaptureImage = image;
    m_lastCapturePath = path;

    const bool openAfterCapture = QSettings().value(SettingsKeys::OpenEditorAfterCapture, true).toBool();
    if (openAfterCapture) {
        openEditorOn(image, path);
    }
}

void FloatingWidget::flashStatus(const QString &text)
{
    m_panel->setVisible(true);
    adjustSize();
    m_statusLabel->setText(text);
    QTimer::singleShot(2500, this, [this]() {
        m_statusLabel->clear();
    });
}

void FloatingWidget::restorePosition()
{
    QSettings settings;
    if (settings.contains(SettingsKeys::PillPosition)) {
        const QPoint saved = settings.value(SettingsKeys::PillPosition).toPoint();
        QRect virtualDesktop;
        for (QScreen *screen : QGuiApplication::screens()) {
            virtualDesktop = virtualDesktop.united(screen->geometry());
        }
        if (virtualDesktop.contains(saved)) {
            move(saved);
            return;
        }
    }

    if (QScreen *primary = QGuiApplication::primaryScreen()) {
        const QRect avail = primary->availableGeometry();
        move(avail.right() - width() - 24, avail.top() + 24);
    }
}

void FloatingWidget::savePosition()
{
    QSettings().setValue(SettingsKeys::PillPosition, pos());
}

void FloatingWidget::loadLastCaptureFromDisk()
{
    const QString picturesDir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    QDir capturesDir(picturesDir + QStringLiteral("/Captures"));
    const QStringList files = capturesDir.entryList(QStringList() << QStringLiteral("capture_*.png"),
                                                      QDir::Files, QDir::Name);
    if (files.isEmpty()) {
        return;
    }
    // Filenames are capture_<timestamp>.png, so lexicographic order is
    // chronological order -- the last entry is the most recent capture.
    m_lastCapturePath = capturesDir.filePath(files.last());
    m_lastCaptureImage = QImage(m_lastCapturePath);
}

void FloatingWidget::openEditorOn(const QImage &image, const QString &path)
{
    auto *editor = new EditorWindow(image, path);
    m_lastEditor = editor;
    editor->show();
}

void FloatingWidget::showLastCaptureInEditor()
{
    if (m_lastCapturePath.isEmpty()) {
        return;
    }
    openEditorOn(m_lastCaptureImage, m_lastCapturePath);
}

void FloatingWidget::showConfigure()
{
    auto *settingsWindow = new SettingsWindow(m_lastEditor ? static_cast<QWidget *>(m_lastEditor) : nullptr);
    settingsWindow->show();
}
