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

// Centralized QSettings key names so FloatingWidget and SettingsWindow can't
// drift out of sync with each other.
namespace SettingsKeys {

constexpr auto OpenEditorAfterCapture = "editor/openAfterCapture";
constexpr auto PillPosition = "pill/position";

} // namespace SettingsKeys
