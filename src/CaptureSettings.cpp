/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        CaptureSettings.cpp
 *  Description: CaptureSettings storage and change notification
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

#include "CaptureSettings.h"

#include "SettingsKeys.h"

#include <QSettings>

namespace {

template <typename T>
void store(const char *key, const T &value, const T &current, CaptureSettings *owner)
{
    if (value == current) {
        return;
    }
    QSettings().setValue(key, value);
    emit owner->changed();
}

} // namespace

CaptureSettings &CaptureSettings::instance()
{
    static CaptureSettings settings;
    return settings;
}

bool CaptureSettings::previewInEditor() const
{
    return QSettings().value(SettingsKeys::OpenEditorAfterCapture, true).toBool();
}

bool CaptureSettings::copyToClipboard() const
{
    return QSettings().value(SettingsKeys::CopyToClipboard, false).toBool();
}

bool CaptureSettings::includeCursor() const
{
    return QSettings().value(SettingsKeys::IncludeCursor, false).toBool();
}

bool CaptureSettings::autoHide() const
{
    return QSettings().value(SettingsKeys::AutoHide, true).toBool();
}

bool CaptureSettings::delayEnabled() const
{
    return QSettings().value(SettingsKeys::DelayEnabled, false).toBool();
}

int CaptureSettings::delaySeconds() const
{
    return qBound(1, QSettings().value(SettingsKeys::DelaySeconds, 5).toInt(), 60);
}

CaptureSettings::Mode CaptureSettings::mode() const
{
    const int raw = QSettings().value(SettingsKeys::CaptureMode, static_cast<int>(Mode::Region)).toInt();
    return raw >= 0 && raw <= static_cast<int>(Mode::AllDisplays) ? static_cast<Mode>(raw) : Mode::Region;
}

void CaptureSettings::setPreviewInEditor(bool on)
{
    store(SettingsKeys::OpenEditorAfterCapture, on, previewInEditor(), this);
}

void CaptureSettings::setCopyToClipboard(bool on)
{
    store(SettingsKeys::CopyToClipboard, on, copyToClipboard(), this);
}

void CaptureSettings::setIncludeCursor(bool on)
{
    store(SettingsKeys::IncludeCursor, on, includeCursor(), this);
}

void CaptureSettings::setAutoHide(bool on)
{
    store(SettingsKeys::AutoHide, on, autoHide(), this);
}

void CaptureSettings::setDelayEnabled(bool on)
{
    store(SettingsKeys::DelayEnabled, on, delayEnabled(), this);
}

void CaptureSettings::setDelaySeconds(int seconds)
{
    store(SettingsKeys::DelaySeconds, qBound(1, seconds, 60), delaySeconds(), this);
}

void CaptureSettings::setMode(Mode mode)
{
    store(SettingsKeys::CaptureMode, static_cast<int>(mode), static_cast<int>(this->mode()), this);
}
