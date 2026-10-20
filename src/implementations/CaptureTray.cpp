/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        CaptureTray.cpp
 *  Description: Slide animation and styling for the capture options tray
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

#include "../headers/CaptureTray.h"

#include "../headers/CaptureOptionsPanel.h"

#include <QEasingCurve>
#include <QPainter>
#include <QVariantAnimation>
#include <QVBoxLayout>
#include <algorithm>

namespace {
constexpr int kGap = 4;
}

CaptureTray::CaptureTray(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);

    m_content = new QWidget(this);
    m_content->setStyleSheet(
        "QLabel, QCheckBox { color: #eeeeee; }"
        "QSpinBox { background: #3d3d3d; color: #eeeeee; border: 1px solid #555; border-radius: 4px; padding: 2px; }"
        "QSpinBox:disabled { color: #888888; }"
        "QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid #888888; border-radius: 3px;"
        " background: #3d3d3d; }"
        "QCheckBox::indicator:checked { background: #3d85c6; border-color: #3d85c6; }");
    auto *layout = new QVBoxLayout(m_content);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->addWidget(new CaptureOptionsPanel(m_content));
    m_content->setFixedSize(m_content->sizeHint());

    m_anim = new QVariantAnimation(this);
    m_anim->setDuration(160);
    m_anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_anim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        applyProgress(v.toReal());
    });
    connect(m_anim, &QVariantAnimation::finished, this, [this]() {
        if (!m_open) {
            hide();
        }
    });
}

void CaptureTray::slideOut(const QRect &anchor, DockEdge edge, const QRect &bounds)
{
    m_anchor = anchor;
    m_edge = edge;

    const QSize full = m_content->size();
    if (edge == DockEdge::Top || edge == DockEdge::Bottom) {
        const int x = std::clamp(anchor.center().x() - full.width() / 2, bounds.left(),
                                 std::max(bounds.left(), bounds.right() - full.width() + 1));
        const int y = edge == DockEdge::Top ? anchor.bottom() + 1 + kGap : anchor.top() - kGap;
        m_origin = QPoint(x, y);
    } else {
        const int y = std::clamp(anchor.center().y() - full.height() / 2, bounds.top(),
                                 std::max(bounds.top(), bounds.bottom() - full.height() + 1));
        const int x = edge == DockEdge::Left ? anchor.right() + 1 + kGap : anchor.left() - kGap;
        m_origin = QPoint(x, y);
    }

    m_open = true;
    m_anim->stop();
    m_anim->setStartValue(m_progress);
    m_anim->setEndValue(1.0);
    applyProgress(m_progress);
    show();
    m_anim->start();
}

void CaptureTray::slideIn()
{
    if (!m_open) {
        return;
    }
    m_open = false;
    m_anim->stop();
    m_anim->setStartValue(m_progress);
    m_anim->setEndValue(0.0);
    m_anim->start();
}

void CaptureTray::closeNow()
{
    m_open = false;
    m_anim->stop();
    m_progress = 0;
    hide();
}

void CaptureTray::applyProgress(qreal t)
{
    m_progress = t;
    const QSize full = m_content->size();
    QRect r;
    QPoint contentPos(0, 0);

    switch (m_edge) {
    case DockEdge::Top: {
        const int th = std::max(1, qRound(full.height() * t));
        r = QRect(m_origin, QSize(full.width(), th));
        contentPos = QPoint(0, th - full.height());
        break;
    }
    case DockEdge::Bottom: {
        const int th = std::max(1, qRound(full.height() * t));
        r = QRect(QPoint(m_origin.x(), m_origin.y() - th), QSize(full.width(), th));
        break;
    }
    case DockEdge::Left: {
        const int th = std::max(1, qRound(full.width() * t));
        r = QRect(m_origin, QSize(th, full.height()));
        contentPos = QPoint(th - full.width(), 0);
        break;
    }
    case DockEdge::Right: {
        const int th = std::max(1, qRound(full.width() * t));
        r = QRect(QPoint(m_origin.x() - th, m_origin.y()), QSize(th, full.height()));
        break;
    }
    }
    setGeometry(r);
    m_content->move(contentPos);
}

void CaptureTray::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(45, 45, 45, 240));
    p.drawRoundedRect(rect(), 12, 12);
}
