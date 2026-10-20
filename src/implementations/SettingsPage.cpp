/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        SettingsPage.cpp
 *  Description: Layout for the editor's settings page
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

#include "../headers/SettingsPage.h"

#include "../headers/CaptureOptionsPanel.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
{
    auto *title = new QLabel(QStringLiteral("Settings"), this);
    QFont f = title->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() * 1.4);
    title->setFont(f);

    auto *done = new QPushButton(QStringLiteral("Done"), this);
    connect(done, &QPushButton::clicked, this, &SettingsPage::doneRequested);

    auto *options = new CaptureOptionsPanel(this);
    options->setMaximumWidth(360);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 24, 32, 24);
    layout->setSpacing(16);
    layout->addWidget(title);
    layout->addWidget(options);
    layout->addStretch();
    layout->addWidget(done, 0, Qt::AlignRight);
}
