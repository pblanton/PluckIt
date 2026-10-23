/*
 * ============================================================================
 *  PluckIt - Screen Capture & Annotation for Linux
 * ============================================================================
 *
 *  File:        FloatingWidget.cpp
 *  Description: Capture widget UI, edge docking, auto-hide, and capture flow
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

#include "../headers/FloatingWidget.h"

#include "../headers/CaptureButton.h"
#include "../headers/CaptureManager.h"
#include "../headers/CaptureSettings.h"
#include "../headers/EditorWindow.h"
#include "../headers/IconFactory.h"
#include "../headers/RegionSelectOverlay.h"
#include "../headers/SettingsKeys.h"
#include "../headers/WindowSelectOverlay.h"

#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDir>
#include <QFileInfo>
#include <QGridLayout>
#include <QGuiApplication>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QScreen>
#include <QSettings>
#include <QStandardPaths>
#include <QCursor>
#include <QTimer>
#include <QToolTip>
#include <QVariantAnimation>
#include <algorithm>

namespace {

constexpr int kEdgeHysteresis = 40;
constexpr int kDragThreshold = 4;

// Auto-hide timing. The expand delay is long enough that sweeping the pointer
// across the handle on the way to something else doesn't pop the widget out;
// the collapse delay forgives brief excursions (e.g. aiming for a tray toggle).
constexpr int kHoverPollMs = 100;
constexpr int kExpandDelayMs = 350;
constexpr int kCollapseDelayMs = 1000;
constexpr int kSlideInMs = 220;
constexpr int kSlideOutMs = 180;
constexpr int kHandleThickness = 10;
constexpr qreal kEdgeCornerRadius = 4;
constexpr qreal kInnerCornerRadius = 14;
constexpr int kHoverSlack = 8;

bool isHorizontal(DockEdge edge)
{
    return edge == DockEdge::Top || edge == DockEdge::Bottom;
}

int alongOf(const QPoint &p, DockEdge edge)
{
    return isHorizontal(edge) ? p.x() : p.y();
}

const QColor kGlyphColor(235, 235, 235);

QPushButton *makeGlyphButton(QWidget *parent, IconFactory::Glyph glyph, const QSize &size, const QString &tip)
{
    auto *button = new QPushButton(parent);
    button->setIcon(IconFactory::glyphIcon(glyph, 24, kGlyphColor));
    button->setIconSize(QSize(24, 24));
    button->setFixedSize(size);
    button->setToolTip(tip);
    button->setFocusPolicy(Qt::NoFocus);
    button->setCursor(Qt::PointingHandCursor);
    button->setStyleSheet("QPushButton { background: transparent; border: none; border-radius: 8px; }"
                          "QPushButton:hover { background: #4a4a4a; }"
                          "QPushButton:pressed { background: #5a5a5a; }");
    return button;
}

QPainterPath roundedRectPath(const QRectF &r, qreal tl, qreal tr, qreal br, qreal bl)
{
    QPainterPath p;
    p.moveTo(r.left() + tl, r.top());
    p.lineTo(r.right() - tr, r.top());
    p.arcTo(r.right() - 2 * tr, r.top(), 2 * tr, 2 * tr, 90, -90);
    p.lineTo(r.right(), r.bottom() - br);
    p.arcTo(r.right() - 2 * br, r.bottom() - 2 * br, 2 * br, 2 * br, 0, -90);
    p.lineTo(r.left() + bl, r.bottom());
    p.arcTo(r.left(), r.bottom() - 2 * bl, 2 * bl, 2 * bl, 270, -90);
    p.lineTo(r.left(), r.top() + tl);
    p.arcTo(r.left(), r.top(), 2 * tl, 2 * tl, 180, -90);
    p.closeSubpath();
    return p;
}

} // namespace

FloatingWidget::FloatingWidget(QWidget *parent)
    : QWidget(parent)
    , m_backend(createCaptureBackend())
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);

    buildUi();
    loadLastCaptureFromDisk();
    m_editButton->setEnabled(!m_lastCapturePath.isEmpty());
    restoreDock();
}

void FloatingWidget::buildUi()
{
    m_viewport = new QWidget(this);
    m_body = new QWidget(m_viewport);
    m_grid = new QGridLayout(m_body);
    m_grid->setContentsMargins(10, 10, 10, 10);
    m_grid->setSpacing(6);
    m_grid->setSizeConstraint(QLayout::SetFixedSize);

    m_gearButton = makeGlyphButton(m_body, IconFactory::Glyph::Gear, QSize(36, 36), QStringLiteral("Settings"));
    m_editButton = makeGlyphButton(m_body, IconFactory::Glyph::Pencil, QSize(36, 36), QStringLiteral("Open editor"));
    m_menuButton = makeGlyphButton(m_body, IconFactory::Glyph::Menu, QSize(36, 24), QStringLiteral("Capture options"));
    m_captureButton = new CaptureButton(m_body);

    m_tray = new CaptureTray(this);
    m_countdownTimer = new QTimer(this);
    m_countdownTimer->setInterval(1000);

    connect(m_gearButton, &QPushButton::clicked, this, &FloatingWidget::showSettings);
    connect(m_editButton, &QPushButton::clicked, this, &FloatingWidget::showLastCaptureInEditor);
    connect(m_menuButton, &QPushButton::clicked, this, &FloatingWidget::toggleTray);
    connect(m_captureButton, &QAbstractButton::clicked, this, &FloatingWidget::onCaptureClicked);
    connect(m_captureButton, &CaptureButton::modeMenuRequested, this, &FloatingWidget::showModeMenu);
    connect(m_countdownTimer, &QTimer::timeout, this, &FloatingWidget::tickCountdown);

    m_slide = new QVariantAnimation(this);
    m_slide->setStartValue(0.0);
    m_slide->setEndValue(1.0);
    m_slide->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_slide, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        applySlide(v.toReal());
    });

    m_hoverTimer = new QTimer(this);
    m_hoverTimer->setInterval(kHoverPollMs);
    connect(m_hoverTimer, &QTimer::timeout, this, &FloatingWidget::pollHover);
    m_hoverTimer->start();
    connect(&CaptureSettings::instance(), &CaptureSettings::changed, this, [this]() {
        if (!CaptureSettings::instance().autoHide() && m_collapsed) {
            expand();
        }
    });

    // Buttons swallow mouse events, so dragging has to be recognised in a
    // filter or the widget could only be moved by its thin margins.
    for (QWidget *w : {static_cast<QWidget *>(m_gearButton), static_cast<QWidget *>(m_editButton),
                       static_cast<QWidget *>(m_menuButton), static_cast<QWidget *>(m_captureButton)}) {
        w->installEventFilter(this);
    }
}

void FloatingWidget::applyLayout()
{
    while (QLayoutItem *item = m_grid->takeAt(0)) {
        delete item;
    }

    // The hamburger sits on the side facing the screen interior, which is also
    // the side the tray slides out toward.
    if (isHorizontal(m_edge)) {
        const bool menuBelow = m_edge == DockEdge::Top;
        const int mainRow = menuBelow ? 0 : 1;
        const int menuRow = menuBelow ? 1 : 0;
        m_grid->addWidget(m_gearButton, mainRow, 0, Qt::AlignCenter);
        m_grid->addWidget(m_captureButton, mainRow, 1, Qt::AlignCenter);
        m_grid->addWidget(m_editButton, mainRow, 2, Qt::AlignCenter);
        m_grid->addWidget(m_menuButton, menuRow, 0, 1, 3, Qt::AlignCenter);
    } else {
        const bool menuRight = m_edge == DockEdge::Left;
        const int mainCol = menuRight ? 0 : 1;
        const int menuCol = menuRight ? 1 : 0;
        m_grid->addWidget(m_gearButton, 0, mainCol, Qt::AlignCenter);
        m_grid->addWidget(m_captureButton, 1, mainCol, Qt::AlignCenter);
        m_grid->addWidget(m_editButton, 2, mainCol, Qt::AlignCenter);
        m_grid->addWidget(m_menuButton, 0, menuCol, 3, 1, Qt::AlignCenter);
    }
    m_grid->activate();
}

void FloatingWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(45, 45, 45, 240));

    // Squarer corners on the side that sits against the screen edge; the
    // interior side softens as the widget opens.
    const QRectF r(rect());
    const qreal limit = qMin(r.width(), r.height()) / 2;
    const qreal edgeR = qMin(kEdgeCornerRadius, limit);
    const qreal innerR = qMin(kEdgeCornerRadius + m_slideT * (kInnerCornerRadius - kEdgeCornerRadius), limit);
    switch (m_edge) {
    case DockEdge::Top: p.drawPath(roundedRectPath(r, edgeR, edgeR, innerR, innerR)); break;
    case DockEdge::Bottom: p.drawPath(roundedRectPath(r, innerR, innerR, edgeR, edgeR)); break;
    case DockEdge::Left: p.drawPath(roundedRectPath(r, edgeR, innerR, innerR, edgeR)); break;
    case DockEdge::Right: p.drawPath(roundedRectPath(r, innerR, edgeR, edgeR, innerR)); break;
    }

    // Grip line down the middle of the edge strip; stays visible when open.
    QRectF strip = r;
    switch (m_edge) {
    case DockEdge::Top: strip.setHeight(kHandleThickness); break;
    case DockEdge::Bottom: strip.setTop(r.bottom() - kHandleThickness); break;
    case DockEdge::Left: strip.setWidth(kHandleThickness); break;
    case DockEdge::Right: strip.setLeft(r.right() - kHandleThickness); break;
    }
    const QPointF c = strip.center();
    p.setBrush(QColor(229, 57, 53));
    if (isHorizontal(m_edge)) {
        p.drawRoundedRect(QRectF(c.x() - 18, c.y() - 1.5, 36, 3), 1.5, 1.5);
    } else {
        p.drawRoundedRect(QRectF(c.x() - 1.5, c.y() - 18, 3, 36), 1.5, 1.5);
    }
}

void FloatingWidget::dockAt(DockEdge edge, QScreen *screen, int alongCenter)
{
    if (!screen) {
        return;
    }
    const bool layoutChanged = edge != m_edge || !m_screen;
    m_edge = edge;
    m_screen = screen;
    if (layoutChanged) {
        applyLayout();
    }

    const QRect area = screen->availableGeometry();
    m_body->resize(m_body->sizeHint());
    QSize size = m_body->size();
    if (isHorizontal(edge)) {
        size.rheight() += kHandleThickness;
    } else {
        size.rwidth() += kHandleThickness;
    }

    int x;
    int y;
    if (isHorizontal(edge)) {
        x = std::clamp(alongCenter - size.width() / 2, area.left(), std::max(area.left(), area.right() - size.width() + 1));
        y = edge == DockEdge::Top ? area.top() : area.bottom() - size.height() + 1;
        m_along = x + size.width() / 2;
    } else {
        y = std::clamp(alongCenter - size.height() / 2, area.top(), std::max(area.top(), area.bottom() - size.height() + 1));
        x = edge == DockEdge::Left ? area.left() : area.right() - size.width() + 1;
        m_along = y + size.height() / 2;
    }
    m_fullRect = QRect(QPoint(x, y), size);
    applySlide(m_collapsed ? 0.0 : 1.0);
}

void FloatingWidget::applySlide(qreal t)
{
    m_slideT = t;
    const QSize full = m_fullRect.size();
    const QSize bodySize = m_body->size();
    const bool horizontal = isHorizontal(m_edge);

    // The window always spans the full length along the dock edge and shrinks
    // toward the edge strip. The body keeps its size, anchored to the inner end
    // of the viewport, so the shrinking window clips it.
    const int fullThick = horizontal ? full.height() : full.width();
    const int thick = qRound(kHandleThickness + t * (fullThick - kHandleThickness));
    const int view = thick - kHandleThickness;

    QRect g;
    QRect v;
    QPoint bodyPos(0, 0);
    switch (m_edge) {
    case DockEdge::Top:
        g = QRect(m_fullRect.left(), m_fullRect.top(), full.width(), thick);
        v = QRect(0, kHandleThickness, full.width(), view);
        bodyPos = QPoint(0, view - bodySize.height());
        break;
    case DockEdge::Bottom:
        g = QRect(m_fullRect.left(), m_fullRect.bottom() - thick + 1, full.width(), thick);
        v = QRect(0, 0, full.width(), view);
        break;
    case DockEdge::Left:
        g = QRect(m_fullRect.left(), m_fullRect.top(), thick, full.height());
        v = QRect(kHandleThickness, 0, view, full.height());
        bodyPos = QPoint(view - bodySize.width(), 0);
        break;
    case DockEdge::Right:
        g = QRect(m_fullRect.right() - thick + 1, m_fullRect.top(), thick, full.height());
        v = QRect(0, 0, view, full.height());
        break;
    }
    setGeometry(g);
    m_viewport->setGeometry(v);
    m_body->move(bodyPos);
    m_viewport->setVisible(view > 0);
    update();
}

void FloatingWidget::pollHover()
{
    const CaptureSettings &settings = CaptureSettings::instance();
    if (!isVisible() || m_dragging || !settings.autoHide() || m_slide->state() == QAbstractAnimation::Running) {
        m_dwell.invalidate();
        return;
    }

    const QPoint cursor = QCursor::pos();
    if (m_collapsed) {
        if (!geometry().contains(cursor)) {
            m_dwell.invalidate();
        } else if (!m_dwell.isValid()) {
            m_dwell.start();
        } else if (m_dwell.elapsed() >= kExpandDelayMs) {
            expand();
        }
        return;
    }

    const int slack = kHoverSlack;
    const bool inside = geometry().adjusted(-slack, -slack, slack, slack).contains(cursor)
        || (m_tray->isVisible() && m_tray->geometry().adjusted(-slack, -slack, slack, slack).contains(cursor));
    // Counting down or showing a menu means the user is mid-task even though
    // the pointer is elsewhere.
    const bool busy = m_captureActive || m_countdownTimer->isActive() || QApplication::activePopupWidget();
    if (inside || busy) {
        m_dwell.invalidate();
    } else if (!m_dwell.isValid()) {
        m_dwell.start();
    } else if (m_dwell.elapsed() >= kCollapseDelayMs) {
        collapse();
    }
}

void FloatingWidget::expand()
{
    m_dwell.invalidate();
    m_collapsed = false;
    m_slide->stop();
    m_slide->setDuration(kSlideInMs);
    m_slide->setStartValue(m_slideT);
    m_slide->setEndValue(1.0);
    m_slide->start();
}

void FloatingWidget::collapse()
{
    m_dwell.invalidate();
    m_tray->closeNow();
    m_collapsed = true;
    m_slide->stop();
    m_slide->setDuration(kSlideOutMs);
    m_slide->setStartValue(m_slideT);
    m_slide->setEndValue(0.0);
    m_slide->start();
}

DockEdge FloatingWidget::pickEdge(const QPoint &globalPos, QScreen *screen) const
{
    const QRect area = screen->availableGeometry();
    const int dist[4] = {
        globalPos.x() - area.left(),   // Left
        area.right() - globalPos.x(),  // Right
        globalPos.y() - area.top(),    // Top
        area.bottom() - globalPos.y(), // Bottom
    };
    const int best = int(std::min_element(dist, dist + 4) - dist);
    // Without some stickiness the edge would flicker whenever the cursor
    // crosses the diagonal between two edges.
    if (screen == m_screen.data() && dist[int(m_edge)] <= dist[best] + kEdgeHysteresis) {
        return m_edge;
    }
    return static_cast<DockEdge>(best);
}

void FloatingWidget::restoreDock()
{
    QSettings settings;
    QScreen *screen = nullptr;
    const QString name = settings.value(SettingsKeys::WidgetScreen).toString();
    for (QScreen *s : QGuiApplication::screens()) {
        if (s->name() == name) {
            screen = s;
        }
    }
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }
    if (!screen) {
        return;
    }

    const int rawEdge = settings.value(SettingsKeys::WidgetEdge, int(DockEdge::Right)).toInt();
    const DockEdge edge = rawEdge >= 0 && rawEdge <= int(DockEdge::Bottom) ? static_cast<DockEdge>(rawEdge)
                                                                            : DockEdge::Right;
    const QRect area = screen->availableGeometry();
    const int fallback = isHorizontal(edge) ? area.center().x() : area.top() + 160;
    dockAt(edge, screen, settings.value(SettingsKeys::WidgetAlong, fallback).toInt());
}

void FloatingWidget::saveDock()
{
    QSettings settings;
    if (m_screen) {
        settings.setValue(SettingsKeys::WidgetScreen, m_screen->name());
    }
    settings.setValue(SettingsKeys::WidgetEdge, int(m_edge));
    settings.setValue(SettingsKeys::WidgetAlong, m_along);
}

void FloatingWidget::dragPress(const QPoint &globalPos)
{
    m_pressGlobalPos = globalPos;
    m_dragging = false;
}

bool FloatingWidget::dragMove(const QPoint &globalPos)
{
    if (!m_dragging) {
        if ((globalPos - m_pressGlobalPos).manhattanLength() <= kDragThreshold) {
            return false;
        }
        m_dragging = true;
        closeTray();
        if (m_collapsed) {
            m_slide->stop();
            m_collapsed = false;
            m_slideT = 1.0;
            applySlide(1.0);
        }
        m_grabOffset = alongOf(globalPos, m_edge) - m_along;
    }

    QScreen *screen = QGuiApplication::screenAt(globalPos);
    if (!screen) {
        screen = m_screen ? m_screen.data() : QGuiApplication::primaryScreen();
    }
    const DockEdge edge = pickEdge(globalPos, screen);
    if (isHorizontal(edge) != isHorizontal(m_edge)) {
        m_grabOffset = 0;
    }
    dockAt(edge, screen, alongOf(globalPos, edge) - m_grabOffset);
    return true;
}

bool FloatingWidget::dragRelease()
{
    if (!m_dragging) {
        return false;
    }
    m_dragging = false;
    saveDock();
    return true;
}

bool FloatingWidget::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        auto *me = static_cast<QMouseEvent *>(event);
        if (me->button() == Qt::LeftButton) {
            dragPress(me->globalPosition().toPoint());
        }
        break;
    }
    case QEvent::MouseMove: {
        auto *me = static_cast<QMouseEvent *>(event);
        if ((me->buttons() & Qt::LeftButton) && dragMove(me->globalPosition().toPoint())) {
            if (auto *button = qobject_cast<QAbstractButton *>(watched)) {
                button->setDown(false);
            }
            return true;
        }
        break;
    }
    case QEvent::MouseButtonRelease:
        if (dragRelease()) {
            return true;
        }
        break;
    default:
        break;
    }
    return QWidget::eventFilter(watched, event);
}

void FloatingWidget::mousePressEvent(QMouseEvent *event)
{
    if (m_collapsed) {
        expand();
        return;
    }
    if (event->button() == Qt::LeftButton) {
        dragPress(event->globalPosition().toPoint());
    }
    QWidget::mousePressEvent(event);
}

void FloatingWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        dragMove(event->globalPosition().toPoint());
    }
    QWidget::mouseMoveEvent(event);
}

void FloatingWidget::mouseReleaseEvent(QMouseEvent *event)
{
    dragRelease();
    QWidget::mouseReleaseEvent(event);
}

void FloatingWidget::hideEvent(QHideEvent *event)
{
    m_tray->closeNow();
    QWidget::hideEvent(event);
}

void FloatingWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    QAction *editorAction = menu.addAction(QStringLiteral("Open Editor"));
    editorAction->setEnabled(!m_lastCapturePath.isEmpty());
    QAction *settingsAction = menu.addAction(QStringLiteral("Settings"));
    menu.addSeparator();
    QAction *exitAction = menu.addAction(QStringLiteral("Exit"));

    QAction *chosen = menu.exec(event->globalPos());
    if (chosen == editorAction) {
        showLastCaptureInEditor();
    } else if (chosen == settingsAction) {
        showSettings();
    } else if (chosen == exitAction) {
        QApplication::quit();
    }
}

void FloatingWidget::toggleTray()
{
    if (m_tray->isOpen()) {
        m_tray->slideIn();
    } else if (m_screen) {
        m_tray->slideOut(geometry(), m_edge, m_screen->availableGeometry());
    }
}

void FloatingWidget::closeTray()
{
    m_tray->slideIn();
}

void FloatingWidget::onCaptureClicked()
{
    if (m_countdownTimer->isActive()) {
        cancelCountdown();
        return;
    }
    closeTray();

    if (CaptureSettings::instance().mode() == CaptureSettings::Mode::Monitor && !chooseMonitor()) {
        return;
    }
    if (CaptureSettings::instance().delayEnabled()) {
        startCountdown();
    } else {
        beginCapture();
    }
}

void FloatingWidget::showModeMenu()
{
    struct Entry {
        CaptureSettings::Mode mode;
        const char *label;
    };
    const Entry entries[] = {
        {CaptureSettings::Mode::Region, "Region"},
        {CaptureSettings::Mode::Window, "Window"},
        {CaptureSettings::Mode::Monitor, "Monitor"},
        {CaptureSettings::Mode::AllDisplays, "All Displays"},
    };

    QMenu menu(this);
    auto *group = new QActionGroup(&menu);
    for (const Entry &entry : entries) {
        QAction *action = menu.addAction(QString::fromLatin1(entry.label));
        action->setCheckable(true);
        action->setChecked(CaptureSettings::instance().mode() == entry.mode);
        group->addAction(action);
        connect(action, &QAction::triggered, this, [mode = entry.mode]() {
            CaptureSettings::instance().setMode(mode);
        });
    }
    menu.exec(m_captureButton->mapToGlobal(QPoint(0, m_captureButton->height())));
}

bool FloatingWidget::chooseMonitor()
{
    const QList<QScreen *> screens = QGuiApplication::screens();
    if (screens.size() == 1) {
        m_pendingMonitor = screens.first()->geometry();
        return true;
    }

    QMenu menu(this);
    for (QScreen *screen : screens) {
        const QRect g = screen->geometry();
        QAction *action = menu.addAction(QStringLiteral("%1 (%2x%3)").arg(screen->name()).arg(g.width()).arg(g.height()));
        action->setData(g);
    }
    QAction *chosen = menu.exec(m_captureButton->mapToGlobal(QPoint(0, m_captureButton->height())));
    if (!chosen) {
        return false;
    }
    m_pendingMonitor = chosen->data().toRect();
    return true;
}

void FloatingWidget::startCountdown()
{
    m_countdownRemaining = CaptureSettings::instance().delaySeconds();
    m_captureButton->setCountdown(m_countdownRemaining);
    m_countdownTimer->start();
}

void FloatingWidget::cancelCountdown()
{
    m_countdownTimer->stop();
    m_captureButton->setCountdown(0);
}

void FloatingWidget::tickCountdown()
{
    if (--m_countdownRemaining > 0) {
        m_captureButton->setCountdown(m_countdownRemaining);
        return;
    }
    cancelCountdown();
    beginCapture();
}

void FloatingWidget::beginCapture()
{
    m_captureActive = true;
    int settleMs = 200;
    if (CaptureSettings::instance().hideWhileCapturing()) {
        hide();
        EditorWindow *editor = EditorWindow::instance();
        if (editor && editor->isVisible() && !editor->isMinimized()) {
            editor->hide();
            m_hiddenEditor = editor;
            settleMs = 300;
        }
    }
    // Give the window manager a moment to actually unmap our windows so they
    // don't end up in the screenshot.
    QTimer::singleShot(settleMs, this, &FloatingWidget::runCapture);
}

void FloatingWidget::endCapture()
{
    m_captureActive = false;
    show();
    if (m_hiddenEditor) {
        m_hiddenEditor->show();
        m_hiddenEditor.clear();
    }
}

void FloatingWidget::runCapture()
{
    switch (CaptureSettings::instance().mode()) {
    case CaptureSettings::Mode::Region: {
        auto *overlay = new RegionSelectOverlay(m_backend->virtualDesktopGeometry());
        connect(overlay, &RegionSelectOverlay::regionSelected, this, &FloatingWidget::onRegionSelected);
        connect(overlay, &RegionSelectOverlay::cancelled, this, &FloatingWidget::onSelectionCancelled);
        overlay->show();
        break;
    }
    case CaptureSettings::Mode::Window: {
        auto *overlay = new WindowSelectOverlay(m_backend.get(), m_backend->virtualDesktopGeometry());
        connect(overlay, &WindowSelectOverlay::windowSelected, this, &FloatingWidget::onWindowSelected);
        connect(overlay, &WindowSelectOverlay::cancelled, this, &FloatingWidget::onSelectionCancelled);
        overlay->show();
        break;
    }
    case CaptureSettings::Mode::Monitor:
        completeCapture(m_pendingMonitor);
        break;
    case CaptureSettings::Mode::AllDisplays:
        completeCapture(m_backend->virtualDesktopGeometry());
        break;
    }
}

void FloatingWidget::onRegionSelected(const QRect &globalRect)
{
    completeCapture(globalRect);
}

void FloatingWidget::onWindowSelected(const WindowInfo &info)
{
    completeCapture(info.geometry);
}

void FloatingWidget::onSelectionCancelled()
{
    endCapture();
}

void FloatingWidget::completeCapture(const QRect &globalRect)
{
    // Grab before re-showing our windows so they can't appear in the shot.
    const QImage image = m_backend->grabRect(globalRect, CaptureSettings::instance().includeCursor());
    endCapture();
    finishCapture(image);
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
    m_editButton->setEnabled(true);

    const CaptureSettings &settings = CaptureSettings::instance();
    if (settings.copyToClipboard()) {
        QGuiApplication::clipboard()->setImage(image);
    }
    if (settings.previewInEditor()) {
        openEditorOn(image, path);
    }
}

void FloatingWidget::flashStatus(const QString &text)
{
    QToolTip::showText(mapToGlobal(rect().center()), text, this, QRect(), 2500);
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
    EditorWindow::acquire().openImage(image, path);
}

void FloatingWidget::showLastCaptureInEditor()
{
    if (m_lastCapturePath.isEmpty()) {
        return;
    }
    // Prefer the file on disk: the user may have saved edits over the original
    // capture since it was taken.
    const QImage onDisk(m_lastCapturePath);
    openEditorOn(onDisk.isNull() ? m_lastCaptureImage : onDisk, m_lastCapturePath);
}

void FloatingWidget::showSettings()
{
    closeTray();
    EditorWindow &editor = EditorWindow::acquire();
    if (!editor.hasImages() && !m_lastCapturePath.isEmpty()) {
        showLastCaptureInEditor();
    }
    editor.showSettingsPage();
    editor.present();
}
