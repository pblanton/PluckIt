/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        EditorWindow.cpp
 *  Description: Editor toolbars, page switching, and save/close handling
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

#include "../headers/EditorWindow.h"

#include "../headers/IconFactory.h"
#include "../headers/SettingsPage.h"

#include <QActionGroup>
#include <QClipboard>
#include <QColorDialog>
#include <QDateTime>
#include <QFileInfo>
#include <QGuiApplication>
#include <QKeySequence>
#include <QScrollArea>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>

QPointer<EditorWindow> EditorWindow::s_instance;

namespace {

const char *kPathProperty = "pluckitPath";

// Tabs are narrow and the filename's distinguishing part is its time, so show
// just that (plus the date for captures from another day).
QString tabTitle(const QString &path)
{
    const QString name = QFileInfo(path).completeBaseName();
    const QDateTime taken = QDateTime::fromString(name, QStringLiteral("'capture_'yyyy-MM-dd_HHmmss"));
    if (!taken.isValid()) {
        return name;
    }
    return taken.date() == QDate::currentDate() ? taken.toString(QStringLiteral("HH:mm:ss"))
                                                 : taken.toString(QStringLiteral("MM-dd HH:mm:ss"));
}

} // namespace

EditorWindow *EditorWindow::instance()
{
    return s_instance;
}

EditorWindow &EditorWindow::acquire()
{
    if (!s_instance) {
        s_instance = new EditorWindow;
    }
    return *s_instance;
}

EditorWindow::EditorWindow()
    : QMainWindow(nullptr)
{
    setAttribute(Qt::WA_DeleteOnClose);

    m_tabs = new QTabWidget(this);
    m_tabs->setDocumentMode(true);
    m_tabs->setTabsClosable(true);
    m_tabs->setMovable(true);
    m_tabs->setElideMode(Qt::ElideRight);
    connect(m_tabs, &QTabWidget::tabCloseRequested, this, &EditorWindow::closeTab);
    connect(m_tabs, &QTabWidget::currentChanged, this, &EditorWindow::onCurrentTabChanged);

    auto *settingsPage = new SettingsPage(this);
    connect(settingsPage, &SettingsPage::doneRequested, this, [this]() {
        if (hasImages()) {
            showImagePage();
        } else {
            close();
        }
    });

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(m_tabs);
    m_stack->addWidget(settingsPage);
    setCentralWidget(m_stack);

    // --- top toolbar: file/document-level actions ---
    auto *topBar = new QToolBar(QStringLiteral("Actions"), this);
    m_topBar = topBar;
    topBar->setMovable(false);
    topBar->setIconSize(QSize(24, 24));
    addToolBar(Qt::TopToolBarArea, topBar);

    m_undoAction = topBar->addAction(IconFactory::icon(IconFactory::Kind::Undo), QStringLiteral("Undo"));
    m_saveAction = topBar->addAction(IconFactory::icon(IconFactory::Kind::Save), QStringLiteral("Save"));
    m_copyAction = topBar->addAction(IconFactory::icon(IconFactory::Kind::Copy), QStringLiteral("Copy"));
    m_copyAction->setShortcut(QKeySequence::Copy);
    m_copyAction->setToolTip(QStringLiteral("Copy the image to the clipboard (%1)")
                                 .arg(QKeySequence(QKeySequence::Copy).toString(QKeySequence::NativeText)));
    topBar->addSeparator();

    m_cropAction = topBar->addAction(IconFactory::icon(IconFactory::Kind::Crop), QStringLiteral("Crop"));
    m_cropAction->setCheckable(true);
    m_applyCropAction = topBar->addAction(QStringLiteral("Apply Crop"));
    m_applyCropAction->setShortcuts({QKeySequence(Qt::Key_Return), QKeySequence(Qt::Key_Enter)});
    topBar->addSeparator();

    m_colorButton = new QToolButton(topBar);
    m_colorButton->setIcon(IconFactory::colorSwatch(m_currentColor, 24));
    m_colorButton->setToolTip(QStringLiteral("Annotation color"));
    topBar->addWidget(m_colorButton);

    auto *settingsAction = topBar->addAction(IconFactory::glyphIcon(IconFactory::Glyph::Gear, 24, QColor(60, 60, 60)),
                                              QStringLiteral("Settings"));
    m_closeTabAction = topBar->addAction(IconFactory::icon(IconFactory::Kind::Close), QStringLiteral("Close tab"));
    m_closeTabAction->setShortcut(QKeySequence::Close);
    m_closeTabAction->setToolTip(QStringLiteral("Close this tab (%1)")
                                     .arg(QKeySequence(QKeySequence::Close).toString(QKeySequence::NativeText)));

    // --- left toolbar: drawing/redaction tools ---
    auto *leftBar = new QToolBar(QStringLiteral("Tools"), this);
    m_leftBar = leftBar;
    leftBar->setMovable(false);
    leftBar->setOrientation(Qt::Vertical);
    leftBar->setIconSize(QSize(28, 28));
    addToolBar(Qt::LeftToolBarArea, leftBar);

    m_widthSpin = new QSpinBox(leftBar);
    m_widthSpin->setRange(1, 20);
    m_widthSpin->setValue(m_strokeWidth);
    m_widthSpin->setToolTip(QStringLiteral("Stroke width"));
    m_widthSpin->setFixedWidth(50);
    leftBar->addWidget(m_widthSpin);
    leftBar->addSeparator();

    m_toolGroup = new QActionGroup(this);
    m_toolGroup->setExclusive(true);

    auto addTool = [&](IconFactory::Kind iconKind, const QString &tip, EditorCanvas::Tool tool) {
        QAction *action = leftBar->addAction(IconFactory::icon(iconKind), tip);
        action->setCheckable(true);
        m_toolGroup->addAction(action);
        connect(action, &QAction::toggled, this, [this, tool](bool on) {
            if (on) {
                m_tool = tool;
                if (EditorCanvas *canvas = currentCanvas()) canvas->setTool(tool);
            }
        });
        return action;
    };

    QAction *arrowAction = addTool(IconFactory::Kind::Arrow, QStringLiteral("Arrow"), EditorCanvas::Tool::Arrow);
    addTool(IconFactory::Kind::Rectangle, QStringLiteral("Rectangle"), EditorCanvas::Tool::Rectangle);
    addTool(IconFactory::Kind::Ellipse, QStringLiteral("Ellipse"), EditorCanvas::Tool::Ellipse);
    leftBar->addSeparator();
    addTool(IconFactory::Kind::Pixelate, QStringLiteral("Pixelate (redact)"), EditorCanvas::Tool::Pixelate);
    addTool(IconFactory::Kind::Blur, QStringLiteral("Blur (redact)"), EditorCanvas::Tool::Blur);

    arrowAction->setChecked(true);

    connect(m_cropAction, &QAction::toggled, this, [this](bool on) {
        if (on) {
            m_tool = EditorCanvas::Tool::Crop;
            if (EditorCanvas *canvas = currentCanvas()) canvas->setTool(m_tool);
        }
    });
    // Entering crop mode and entering a draw/redact tool are mutually exclusive.
    connect(m_toolGroup, &QActionGroup::triggered, this, [this](QAction *) {
        m_cropAction->setChecked(false);
    });
    connect(m_cropAction, &QAction::toggled, this, [this](bool on) {
        if (on && m_toolGroup->checkedAction()) {
            m_toolGroup->checkedAction()->setChecked(false);
        }
    });

    connect(m_widthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int width) {
        m_strokeWidth = width;
        if (EditorCanvas *canvas = currentCanvas()) canvas->setStrokeWidth(width);
    });
    connect(m_colorButton, &QToolButton::clicked, this, &EditorWindow::pickColor);
    connect(m_applyCropAction, &QAction::triggered, this, [this]() {
        if (EditorCanvas *canvas = currentCanvas()) canvas->applyCrop();
        m_cropAction->setChecked(false);
    });
    connect(m_undoAction, &QAction::triggered, this, [this]() {
        if (EditorCanvas *canvas = currentCanvas()) canvas->undo();
    });
    connect(m_saveAction, &QAction::triggered, this, &EditorWindow::saveToFile);
    connect(m_copyAction, &QAction::triggered, this, &EditorWindow::copyToClipboard);
    connect(settingsAction, &QAction::triggered, this, &EditorWindow::showSettingsPage);
    connect(m_closeTabAction, &QAction::triggered, this, [this]() { closeTab(m_tabs->currentIndex()); });

    resize(900, 640);
    updateActionStates();
    showSettingsPage();
}

EditorCanvas *EditorWindow::currentCanvas() const
{
    auto *area = qobject_cast<QScrollArea *>(m_tabs->currentWidget());
    return area ? qobject_cast<EditorCanvas *>(area->widget()) : nullptr;
}

QString EditorWindow::currentPath() const
{
    QWidget *page = m_tabs->currentWidget();
    return page ? page->property(kPathProperty).toString() : QString();
}

bool EditorWindow::hasImages() const
{
    return m_tabs->count() > 0;
}

void EditorWindow::applyToolState(EditorCanvas *canvas)
{
    canvas->setColor(m_currentColor);
    canvas->setStrokeWidth(m_strokeWidth);
    canvas->setTool(m_tool);
}

void EditorWindow::openImage(const QImage &image, const QString &savedPath)
{
    if (image.isNull()) {
        return;
    }

    for (int i = 0; i < m_tabs->count(); ++i) {
        if (!savedPath.isEmpty() && m_tabs->widget(i)->property(kPathProperty).toString() == savedPath) {
            m_tabs->setCurrentIndex(i);
            showImagePage();
            present();
            return;
        }
    }

    auto *canvas = new EditorCanvas(image);
    applyToolState(canvas);
    connect(canvas, &EditorCanvas::changed, this, &EditorWindow::updateActionStates);

    auto *area = new QScrollArea;
    area->setWidget(canvas);
    area->setWidgetResizable(false);
    area->setProperty(kPathProperty, savedPath);

    const int index = m_tabs->addTab(area, tabTitle(savedPath));
    m_tabs->setTabToolTip(index, savedPath);
    m_tabs->setCurrentIndex(index);

    if (m_tabs->count() == 1) {
        resize(qMin(image.width() + 80, 1200), qMin(image.height() + 140, 900));
    }
    showImagePage();
    present();
}

void EditorWindow::closeTab(int index)
{
    if (index < 0 || index >= m_tabs->count()) {
        return;
    }
    QWidget *page = m_tabs->widget(index);
    m_tabs->removeTab(index);
    delete page;

    if (!hasImages() && m_stack->currentIndex() == 0) {
        close();
    }
}

void EditorWindow::onCurrentTabChanged()
{
    if (EditorCanvas *canvas = currentCanvas()) {
        applyToolState(canvas);
    }
    updateActionStates();
    updateWindowTitle();
}

void EditorWindow::present()
{
    if (isMinimized()) {
        showNormal();
    } else {
        show();
    }
    raise();
    activateWindow();
}

void EditorWindow::updateWindowTitle()
{
    if (m_stack->currentIndex() == 1) {
        setWindowTitle(QStringLiteral("PluckIt Settings"));
        return;
    }
    const QString path = currentPath();
    setWindowTitle(path.isEmpty() ? QStringLiteral("PluckIt Editor")
                                  : QStringLiteral("PluckIt Editor - %1").arg(QFileInfo(path).fileName()));
}

void EditorWindow::showSettingsPage()
{
    m_topBar->hide();
    m_leftBar->hide();
    m_stack->setCurrentIndex(1);
    updateWindowTitle();
}

void EditorWindow::showImagePage()
{
    m_topBar->show();
    m_leftBar->show();
    m_stack->setCurrentIndex(0);
    updateWindowTitle();
}

void EditorWindow::updateActionStates()
{
    EditorCanvas *canvas = currentCanvas();
    const bool hasCanvas = canvas != nullptr;
    m_applyCropAction->setEnabled(hasCanvas && canvas->hasPendingCrop());
    m_undoAction->setEnabled(hasCanvas && canvas->canUndo());
    m_saveAction->setEnabled(hasCanvas);
    m_copyAction->setEnabled(hasCanvas);
    m_cropAction->setEnabled(hasCanvas);
    m_closeTabAction->setEnabled(hasCanvas);
}

void EditorWindow::pickColor()
{
    const QColor chosen = QColorDialog::getColor(m_currentColor, this, QStringLiteral("Annotation Color"));
    if (chosen.isValid()) {
        m_currentColor = chosen;
        if (EditorCanvas *canvas = currentCanvas()) canvas->setColor(chosen);
        m_colorButton->setIcon(IconFactory::colorSwatch(chosen, 24));
    }
}

void EditorWindow::copyToClipboard()
{
    EditorCanvas *canvas = currentCanvas();
    if (!canvas) {
        return;
    }
    QGuiApplication::clipboard()->setImage(canvas->flattenedImage());
    statusBar()->showMessage(QStringLiteral("Copied to clipboard"), 2000);
}

void EditorWindow::saveToFile()
{
    EditorCanvas *canvas = currentCanvas();
    if (!canvas) {
        return;
    }
    if (canvas->flattenedImage().save(currentPath(), "PNG")) {
        statusBar()->showMessage(QStringLiteral("Saved"), 2000);
    } else {
        statusBar()->showMessage(QStringLiteral("Save failed"), 3000);
    }
}
