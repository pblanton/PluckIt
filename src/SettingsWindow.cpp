/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        SettingsWindow.cpp
 *  Description: Settings dialog controls, backed directly by QSettings
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

#include "SettingsWindow.h"

#include "SettingsKeys.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QSettings>
#include <QVBoxLayout>

SettingsWindow::SettingsWindow(QWidget *parent)
    : QDialog(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(QStringLiteral("PluckIt Settings"));

    QSettings settings;
    m_openEditorCheck = new QCheckBox(QStringLiteral("Open editor automatically after capture"), this);
    m_openEditorCheck->setChecked(settings.value(SettingsKeys::OpenEditorAfterCapture, true).toBool());
    connect(m_openEditorCheck, &QCheckBox::toggled, this, [](bool on) {
        QSettings().setValue(SettingsKeys::OpenEditorAfterCapture, on);
    });

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &SettingsWindow::close);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsWindow::close);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_openEditorCheck);
    layout->addWidget(buttons);
}
