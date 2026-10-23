/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        CaptureOptionsPanel.cpp
 *  Description: Layout and CaptureSettings bindings for the option rows
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

#include "../headers/CaptureOptionsPanel.h"

#include "../headers/CaptureSettings.h"
#include "../headers/ToggleSwitch.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QSpinBox>

CaptureOptionsPanel::CaptureOptionsPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *grid = new QGridLayout(this);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(8);

    m_preview = new ToggleSwitch(this);
    m_clipboard = new ToggleSwitch(this);
    m_cursor = new ToggleSwitch(this);
    m_autoHide = new ToggleSwitch(this);
    m_hideWhileCapturing = new ToggleSwitch(this);

    grid->addWidget(new QLabel(QStringLiteral("Open Capture in Editor"), this), 0, 0);
    grid->addWidget(m_preview, 0, 1, Qt::AlignRight);
    grid->addWidget(new QLabel(QStringLiteral("Copy to Clipboard"), this), 1, 0);
    grid->addWidget(m_clipboard, 1, 1, Qt::AlignRight);
    grid->addWidget(new QLabel(QStringLiteral("Include Cursor"), this), 2, 0);
    grid->addWidget(m_cursor, 2, 1, Qt::AlignRight);

    grid->addWidget(new QLabel(QStringLiteral("Auto-hide at Edge"), this), 3, 0);
    grid->addWidget(m_autoHide, 3, 1, Qt::AlignRight);

    m_delayCheck = new QCheckBox(QStringLiteral("Capture Delay"), this);
    m_delaySpin = new QSpinBox(this);
    m_delaySpin->setRange(1, 60);
    m_delaySpin->setSuffix(QStringLiteral(" seconds"));
    grid->addWidget(new QLabel(QStringLiteral("Hide PluckIt When Capturing"), this), 4, 0);
    grid->addWidget(m_hideWhileCapturing, 4, 1, Qt::AlignRight);
    grid->addWidget(m_delayCheck, 5, 0);
    grid->addWidget(m_delaySpin, 5, 1, Qt::AlignRight);

    CaptureSettings &settings = CaptureSettings::instance();
    connect(m_preview, &QAbstractButton::toggled, &settings, &CaptureSettings::setPreviewInEditor);
    connect(m_clipboard, &QAbstractButton::toggled, &settings, &CaptureSettings::setCopyToClipboard);
    connect(m_cursor, &QAbstractButton::toggled, &settings, &CaptureSettings::setIncludeCursor);
    connect(m_autoHide, &QAbstractButton::toggled, &settings, &CaptureSettings::setAutoHide);
    connect(m_hideWhileCapturing, &QAbstractButton::toggled, &settings, &CaptureSettings::setHideWhileCapturing);
    connect(m_delayCheck, &QCheckBox::toggled, &settings, &CaptureSettings::setDelayEnabled);
    connect(m_delaySpin, QOverload<int>::of(&QSpinBox::valueChanged), &settings, &CaptureSettings::setDelaySeconds);
    connect(&settings, &CaptureSettings::changed, this, &CaptureOptionsPanel::refresh);

    refresh();
}

void CaptureOptionsPanel::refresh()
{
    const CaptureSettings &settings = CaptureSettings::instance();
    const QSignalBlocker b1(m_preview), b2(m_clipboard), b3(m_cursor), b4(m_delayCheck), b5(m_delaySpin),
        b6(m_autoHide), b7(m_hideWhileCapturing);
    m_preview->setChecked(settings.previewInEditor());
    m_clipboard->setChecked(settings.copyToClipboard());
    m_cursor->setChecked(settings.includeCursor());
    m_autoHide->setChecked(settings.autoHide());
    m_hideWhileCapturing->setChecked(settings.hideWhileCapturing());
    m_delayCheck->setChecked(settings.delayEnabled());
    m_delaySpin->setValue(settings.delaySeconds());
    m_delaySpin->setEnabled(settings.delayEnabled());
}
