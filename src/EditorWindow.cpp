/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        EditorWindow.cpp
 *  Description: Editor window toolbars, actions, and save/close handling
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

#include "EditorWindow.h"

#include "EditorCanvas.h"
#include "IconFactory.h"

#include <QActionGroup>
#include <QColorDialog>
#include <QFileInfo>
#include <QKeySequence>
#include <QLabel>
#include <QScrollArea>
#include <QSpinBox>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>

EditorWindow::EditorWindow(const QImage &image, const QString &savedPath, QWidget *parent)
    : QMainWindow(parent)
    , m_savedPath(savedPath)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(QStringLiteral("PluckIt Editor - %1").arg(QFileInfo(savedPath).fileName()));

    m_canvas = new EditorCanvas(image, this);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidget(m_canvas);
    scrollArea->setWidgetResizable(false);
    setCentralWidget(scrollArea);

    // --- top toolbar: file/document-level actions ---
    auto *topBar = new QToolBar(QStringLiteral("Actions"), this);
    topBar->setMovable(false);
    topBar->setIconSize(QSize(24, 24));
    addToolBar(Qt::TopToolBarArea, topBar);

    m_undoAction = topBar->addAction(IconFactory::icon(IconFactory::Kind::Undo), QStringLiteral("Undo"));
    auto *saveAction = topBar->addAction(IconFactory::icon(IconFactory::Kind::Save), QStringLiteral("Save"));
    topBar->addSeparator();

    auto *cropAction = topBar->addAction(IconFactory::icon(IconFactory::Kind::Crop), QStringLiteral("Crop"));
    cropAction->setCheckable(true);
    m_applyCropAction = topBar->addAction(QStringLiteral("Apply Crop"));
    m_applyCropAction->setShortcuts({QKeySequence(Qt::Key_Return), QKeySequence(Qt::Key_Enter)});
    topBar->addSeparator();

    m_colorButton = new QToolButton(topBar);
    m_colorButton->setIcon(IconFactory::colorSwatch(m_currentColor, 24));
    m_colorButton->setToolTip(QStringLiteral("Annotation color"));
    topBar->addWidget(m_colorButton);

    auto *closeAction = topBar->addAction(IconFactory::icon(IconFactory::Kind::Close), QStringLiteral("Close"));

    // --- left toolbar: drawing/redaction tools ---
    auto *leftBar = new QToolBar(QStringLiteral("Tools"), this);
    leftBar->setMovable(false);
    leftBar->setOrientation(Qt::Vertical);
    leftBar->setIconSize(QSize(28, 28));
    addToolBar(Qt::LeftToolBarArea, leftBar);

    auto *widthSpin = new QSpinBox(leftBar);
    widthSpin->setRange(1, 20);
    widthSpin->setValue(3);
    widthSpin->setToolTip(QStringLiteral("Stroke width"));
    widthSpin->setFixedWidth(50);
    leftBar->addWidget(widthSpin);
    leftBar->addSeparator();

    auto *toolGroup = new QActionGroup(this);
    toolGroup->setExclusive(true);

    auto addTool = [&](IconFactory::Kind iconKind, const QString &tip, EditorCanvas::Tool tool) {
        QAction *action = leftBar->addAction(IconFactory::icon(iconKind), tip);
        action->setCheckable(true);
        toolGroup->addAction(action);
        connect(action, &QAction::toggled, this, [this, tool](bool on) {
            if (on) m_canvas->setTool(tool);
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

    connect(cropAction, &QAction::toggled, this, [this](bool on) {
        if (on) m_canvas->setTool(EditorCanvas::Tool::Crop);
    });
    // Entering crop mode and entering a draw/redact tool are mutually exclusive.
    connect(toolGroup, &QActionGroup::triggered, this, [cropAction](QAction *) {
        cropAction->setChecked(false);
    });
    connect(cropAction, &QAction::toggled, this, [toolGroup](bool on) {
        if (on && toolGroup->checkedAction()) {
            toolGroup->checkedAction()->setChecked(false);
        }
    });

    connect(widthSpin, QOverload<int>::of(&QSpinBox::valueChanged), m_canvas, &EditorCanvas::setStrokeWidth);
    connect(m_colorButton, &QToolButton::clicked, this, &EditorWindow::pickColor);
    connect(m_applyCropAction, &QAction::triggered, this, [this, cropAction]() {
        m_canvas->applyCrop();
        cropAction->setChecked(false);
    });
    connect(m_undoAction, &QAction::triggered, this, [this]() { m_canvas->undo(); });
    connect(saveAction, &QAction::triggered, this, &EditorWindow::saveToFile);
    connect(closeAction, &QAction::triggered, this, &EditorWindow::close);
    connect(m_canvas, &EditorCanvas::changed, this, &EditorWindow::updateActionStates);

    updateActionStates();

    const int w = qMin(image.width() + 80, 1200);
    const int h = qMin(image.height() + 110, 900);
    resize(w, h);
}

void EditorWindow::updateActionStates()
{
    m_applyCropAction->setEnabled(m_canvas->hasPendingCrop());
    m_undoAction->setEnabled(m_canvas->canUndo());
}

void EditorWindow::pickColor()
{
    const QColor chosen = QColorDialog::getColor(m_currentColor, this, QStringLiteral("Annotation Color"));
    if (chosen.isValid()) {
        m_currentColor = chosen;
        m_canvas->setColor(chosen);
        m_colorButton->setIcon(IconFactory::colorSwatch(chosen, 24));
    }
}

void EditorWindow::saveToFile()
{
    const QImage flattened = m_canvas->flattenedImage();
    if (flattened.save(m_savedPath, "PNG")) {
        statusBar()->showMessage(QStringLiteral("Saved"), 2000);
    } else {
        statusBar()->showMessage(QStringLiteral("Save failed"), 3000);
    }
}
