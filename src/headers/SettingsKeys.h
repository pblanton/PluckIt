/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        SettingsKeys.h
 *  Description: Centralized QSettings key names
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

// Centralized QSettings key names so the capture widget, the slide-out tray and
// the editor's settings page can't drift out of sync with each other.
namespace SettingsKeys {

constexpr auto OpenEditorAfterCapture = "editor/openAfterCapture";
constexpr auto CopyToClipboard = "capture/copyToClipboard";
constexpr auto IncludeCursor = "capture/includeCursor";
constexpr auto DelayEnabled = "capture/delayEnabled";
constexpr auto DelaySeconds = "capture/delaySeconds";
constexpr auto CaptureMode = "capture/mode";
constexpr auto WidgetScreen = "widget/screen";
constexpr auto WidgetEdge = "widget/edge";
constexpr auto WidgetAlong = "widget/along";
constexpr auto AutoHide = "widget/autoHide";
constexpr auto HideWhileCapturing = "capture/hideWhileCapturing";

} // namespace SettingsKeys
