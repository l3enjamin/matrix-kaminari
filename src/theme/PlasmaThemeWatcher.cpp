#include "theme/PlasmaThemeWatcher.h"

#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QGuiApplication>
#include <QStandardPaths>
#include <QStyleHints>

#ifdef KAMINARI_ENABLE_KDE
#include <KColorScheme>
#include <KConfigGroup>
#include <KSharedConfig>
#if __has_include(<KConfigWatcher>)
#include <KConfigWatcher>
#endif
#endif

#include <algorithm>
#include <cmath>

namespace {

double linearChannel(double c)
{
    return c <= 0.03928 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
}

double relativeLuminance(const QColor &c)
{
    if (!c.isValid())
        return 0.0;
    return 0.2126 * linearChannel(c.redF())
         + 0.7152 * linearChannel(c.greenF())
         + 0.0722 * linearChannel(c.blueF());
}

double contrastRatio(const QColor &a, const QColor &b)
{
    const double la = relativeLuminance(a) + 0.05;
    const double lb = relativeLuminance(b) + 0.05;
    return std::max(la, lb) / std::min(la, lb);
}

QColor blendOpaque(const QColor &base, const QColor &overlay, double t)
{
    const double a = std::clamp(t, 0.0, 1.0);
    return QColor::fromRgbF(
        base.redF() * (1.0 - a) + overlay.redF() * a,
        base.greenF() * (1.0 - a) + overlay.greenF() * a,
        base.blueF() * (1.0 - a) + overlay.blueF() * a,
        1.0);
}

QColor bestInkFor(const QColor &bg)
{
    const QColor white(0xFF, 0xFF, 0xFF);
    const QColor ink(0x10, 0x12, 0x16);
    return contrastRatio(bg, white) >= contrastRatio(bg, ink) ? white : ink;
}

QColor ensureContrast(const QColor &fg, const QColor &bg, const QColor &anchor, double minRatio)
{
    if (contrastRatio(fg, bg) >= minRatio)
        return fg;
    for (int i = 1; i <= 20; ++i) {
        const QColor candidate = blendOpaque(fg, anchor, double(i) / 20.0);
        if (contrastRatio(candidate, bg) >= minRatio)
            return candidate;
    }
    return anchor;
}

QColor validOr(const QColor &c, const QColor &fallback)
{
    return c.isValid() ? QColor(c.red(), c.green(), c.blue()) : fallback;
}

} // namespace

struct PlasmaThemeWatcher::KdeImpl {
#ifdef KAMINARI_ENABLE_KDE
    KSharedConfigPtr config;
#if __has_include(<KConfigWatcher>)
    KConfigWatcher::Ptr watcher;
#endif
#endif
};

PlasmaThemeWatcher::DerivedPalette PlasmaThemeWatcher::derivePalette(
    const PaletteInput &in)
{
    DerivedPalette out;
    out.schemeName = in.schemeName.trimmed();

    const QColor fallbackBg(0x20, 0x23, 0x26);
    const QColor fallbackView(0x14, 0x16, 0x18);
    const QColor fallbackFg(0xEF, 0xF0, 0xF1);
    const QColor fallbackAccent(0x3D, 0xAE, 0xE9);

    const QColor windowBg = validOr(in.windowBg, fallbackBg);
    const QColor windowFg = validOr(in.windowFg, fallbackFg);
    const QColor viewBg = validOr(in.viewBg, windowBg);
    const QColor viewFg = validOr(in.viewFg, windowFg);
    const QColor viewAltBg = validOr(in.viewAltBg, viewBg);
    const QColor windowAltBg = validOr(in.windowAltBg, windowBg);
    const QColor buttonBg = validOr(in.buttonBg, windowBg);
    const QColor accent = validOr(in.focusColor.isValid() ? in.focusColor : in.selectionBg,
                                  fallbackAccent);
    const QColor selectionBase = validOr(in.selectionBg, accent);
    const QColor hoverBase = validOr(in.hoverColor, accent);

    const bool dark = relativeLuminance(windowBg) < 0.22;
    out.isDark = dark;

    out.background = dark
        ? blendOpaque(viewBg, windowBg, 0.28)
        : blendOpaque(windowBg, viewBg, 0.35);
    out.sidebar = windowBg;
    out.rail = dark
        ? blendOpaque(windowBg, QColor(0x00, 0x00, 0x00), 0.24)
        : blendOpaque(windowBg, windowFg, 0.06);
    out.surface = dark
        ? blendOpaque(windowBg, buttonBg, 0.45)
        : blendOpaque(windowBg, viewBg, 0.72);
    out.cardElevated = dark
        ? blendOpaque(out.surface, windowFg, 0.06)
        : blendOpaque(windowBg, viewBg, 0.50);
    out.inputBg = viewBg;
    out.codeBlock = dark
        ? blendOpaque(viewBg, QColor(0x00, 0x00, 0x00), 0.28)
        : blendOpaque(windowAltBg, windowBg, 0.40);

    out.textPrimary = ensureContrast(viewFg, out.background,
                                     dark ? QColor(0xF8, 0xFA, 0xFC) : QColor(0x11, 0x13, 0x18),
                                     7.0);
    const QColor rawSecondary = validOr(in.windowFg, out.textPrimary);
    out.textSecondary = ensureContrast(
        blendOpaque(rawSecondary, out.textPrimary, 0.30), out.surface, out.textPrimary, 4.8);
    const QColor rawMuted = validOr(in.viewInactiveFg.isValid() ? in.viewInactiveFg : in.windowInactiveFg,
                                    blendOpaque(out.textPrimary, out.background, 0.42));
    out.textMuted = ensureContrast(rawMuted, out.background, out.textPrimary, 4.6);
    out.textDisabled = ensureContrast(
        blendOpaque(out.textMuted, out.background, 0.38), out.background, out.textPrimary, 2.5);

    out.hover = dark
        ? blendOpaque(out.surface, hoverBase, 0.20)
        : blendOpaque(out.surface, hoverBase, 0.14);
    out.selected = dark
        ? blendOpaque(out.surface, selectionBase, 0.34)
        : blendOpaque(out.surface, selectionBase, 0.22);
    out.selectedHover = dark
        ? blendOpaque(out.selected, selectionBase, 0.24)
        : blendOpaque(out.selected, selectionBase, 0.18);
    out.selectedText = ensureContrast(out.textPrimary, out.selected,
                                      bestInkFor(out.selected), 5.0);

    out.border = blendOpaque(out.surface, out.textPrimary, dark ? 0.16 : 0.14);
    out.borderStrong = blendOpaque(out.surface, out.textPrimary, dark ? 0.30 : 0.26);

    out.accent = accent;
    out.accentHover = dark
        ? blendOpaque(accent, QColor(0xFF, 0xFF, 0xFF), 0.14)
        : blendOpaque(accent, QColor(0x00, 0x00, 0x00), 0.10);
    out.accentPressed = blendOpaque(accent, QColor(0x00, 0x00, 0x00), dark ? 0.16 : 0.20);
    out.accentText = validOr(in.selectionFg, bestInkFor(accent));
    if (contrastRatio(out.accent, out.accentText) < 4.5)
        out.accentText = bestInkFor(accent);
    out.accentSoft = blendOpaque(out.background, accent, dark ? 0.18 : 0.14);
    out.accentBorder = blendOpaque(out.border, accent, 0.45);

    out.ownBubble = blendOpaque(out.surface, accent, dark ? 0.72 : 0.80);
    out.ownBubbleText = bestInkFor(out.ownBubble);
    out.otherBubble = out.surface;
    out.reaction = viewAltBg != viewBg ? viewAltBg : out.cardElevated;

    out.link = ensureContrast(validOr(in.linkFg, accent), out.background,
                              dark ? QColor(0x93, 0xC5, 0xFD) : QColor(0x00, 0x5F, 0xB5), 4.5);
    out.positive = ensureContrast(
        validOr(in.positiveFg, dark ? QColor(0x63, 0xD6, 0xA3) : QColor(0x05, 0x64, 0x35)),
        out.background, dark ? QColor(0x63, 0xD6, 0xA3) : QColor(0x05, 0x64, 0x35), 4.5);
    out.warning = ensureContrast(
        validOr(in.warningFg, dark ? QColor(0xFF, 0xAD, 0x67) : QColor(0x81, 0x4A, 0x00)),
        out.background, dark ? QColor(0xFF, 0xAD, 0x67) : QColor(0x81, 0x4A, 0x00), 4.5);
    out.negative = ensureContrast(
        validOr(in.negativeFg, dark ? QColor(0xFF, 0xA7, 0xAF) : QColor(0x9F, 0x2A, 0x30)),
        out.background, dark ? QColor(0xFF, 0xA7, 0xAF) : QColor(0x9F, 0x2A, 0x30), 4.5);
    out.info = out.link;
    out.mention = validOr(in.negativeFg, QColor(0xE0, 0x48, 0x48));
    out.dangerTint = out.mention;
    return out;
}

PlasmaThemeWatcher::PlasmaThemeWatcher(QObject *parent)
    : QObject(parent)
    , m_kde(std::make_unique<KdeImpl>())
{
    m_refreshTimer.setSingleShot(true);
    m_refreshTimer.setInterval(40);
    connect(&m_refreshTimer, &QTimer::timeout,
            this, &PlasmaThemeWatcher::reloadFromDesktop);

    if (auto *gui = qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
        if (auto *hints = gui->styleHints()) {
            connect(hints, &QStyleHints::colorSchemeChanged,
                    this, [this](Qt::ColorScheme) { scheduleRefresh(); });
        }
        gui->installEventFilter(this);
    }

#ifdef KAMINARI_ENABLE_KDE
    const QString cfgDir = QStandardPaths::writableLocation(
        QStandardPaths::GenericConfigLocation);
    if (!cfgDir.isEmpty())
        m_kdeGlobalsPath = cfgDir + QStringLiteral("/kdeglobals");

    m_kde->config = KSharedConfig::openConfig(QStringLiteral("kdeglobals"));
#if __has_include(<KConfigWatcher>)
    m_kde->watcher = KConfigWatcher::create(m_kde->config);
    if (m_kde->watcher) {
        connect(m_kde->watcher.data(), &KConfigWatcher::configChanged,
                this, [this](const KConfigGroup &, const QByteArrayList &) {
                    scheduleRefresh();
                });
    }
#endif
    if (!m_kdeGlobalsPath.isEmpty()) {
        m_fileWatcher = new QFileSystemWatcher(this);
        ensureKdeGlobalsWatched();
        connect(m_fileWatcher, &QFileSystemWatcher::fileChanged,
                this, [this](const QString &) {
                    ensureKdeGlobalsWatched();
                    scheduleRefresh();
                });
        connect(m_fileWatcher, &QFileSystemWatcher::directoryChanged,
                this, [this](const QString &) {
                    ensureKdeGlobalsWatched();
                    scheduleRefresh();
                });
    }
#endif

    reloadFromDesktop();
}

PlasmaThemeWatcher::~PlasmaThemeWatcher() = default;

bool PlasmaThemeWatcher::kdeIntegrationAvailable() const
{
#ifdef KAMINARI_ENABLE_KDE
    return true;
#else
    return false;
#endif
}

bool PlasmaThemeWatcher::eventFilter(QObject *watched, QEvent *event)
{
    if (!m_applyingPalette && !m_testOverride && event
        && (event->type() == QEvent::ApplicationPaletteChange
            || event->type() == QEvent::ThemeChange)) {
        scheduleRefresh();
    }
    return QObject::eventFilter(watched, event);
}

void PlasmaThemeWatcher::scheduleRefresh()
{
    if (!m_testOverride)
        m_refreshTimer.start();
}

void PlasmaThemeWatcher::ensureKdeGlobalsWatched()
{
    if (!m_fileWatcher || m_kdeGlobalsPath.isEmpty())
        return;
    const QString dirPath = QFileInfo(m_kdeGlobalsPath).absolutePath();
    if (QFileInfo::exists(dirPath) && !m_fileWatcher->directories().contains(dirPath))
        m_fileWatcher->addPath(dirPath);
    if (QFileInfo::exists(m_kdeGlobalsPath)
        && !m_fileWatcher->files().contains(m_kdeGlobalsPath)) {
        m_fileWatcher->addPath(m_kdeGlobalsPath);
    }
}

void PlasmaThemeWatcher::refresh()
{
    m_testOverride = false;
    reloadFromDesktop();
}

void PlasmaThemeWatcher::applyTestPaletteInput(const PaletteInput &in, bool active)
{
    m_testOverride = true;
    const DerivedPalette next = derivePalette(in);
    if (m_active == active && m_palette == next)
        return;
    m_active = active;
    m_palette = next;
    Q_EMIT themeChanged();
}

void PlasmaThemeWatcher::applyApplicationPalette()
{
#ifdef KAMINARI_ENABLE_KDE
    if (!m_active || !qobject_cast<QGuiApplication *>(QCoreApplication::instance()))
        return;
    if (!m_applicationPalette.color(QPalette::Window).isValid())
        return;
    m_applyingPalette = true;
    QGuiApplication::setPalette(m_applicationPalette);
    m_applyingPalette = false;
#endif
}

void PlasmaThemeWatcher::reloadFromDesktop()
{
    if (m_testOverride)
        return;

    bool nextActive = false;
    DerivedPalette nextPalette;
    QPalette nextAppPalette;

#ifdef KAMINARI_ENABLE_KDE
    const bool disabled = qEnvironmentVariableIsSet("KAMINARI_DISABLE_PLASMA_THEME");
    const bool hasGui =
        qobject_cast<QGuiApplication *>(QCoreApplication::instance()) != nullptr;
    const QString desktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP");
    const bool inPlasmaSession =
        desktop.contains(QLatin1String("KDE"), Qt::CaseInsensitive)
        || qEnvironmentVariable("KDE_FULL_SESSION") == QLatin1String("true")
        || (!m_kdeGlobalsPath.isEmpty() && QFileInfo::exists(m_kdeGlobalsPath))
        || qEnvironmentVariableIsSet("KAMINARI_FORCE_PLASMA_THEME");

    if (!disabled && hasGui && inPlasmaSession && m_kde->config) {
        m_kde->config->reparseConfiguration();
        const KConfigGroup general(m_kde->config, QStringLiteral("General"));
        const QString scheme = general.readEntry(QStringLiteral("ColorScheme"),
                                                 QStringLiteral("Breeze"));

        const KColorScheme windowScheme(QPalette::Active, KColorScheme::Window, m_kde->config);
        const KColorScheme inactiveWindow(QPalette::Inactive, KColorScheme::Window, m_kde->config);
        const KColorScheme viewScheme(QPalette::Active, KColorScheme::View, m_kde->config);
        const KColorScheme inactiveView(QPalette::Inactive, KColorScheme::View, m_kde->config);
        const KColorScheme buttonScheme(QPalette::Active, KColorScheme::Button, m_kde->config);
        const KColorScheme selectionScheme(QPalette::Active, KColorScheme::Selection, m_kde->config);

        PaletteInput in;
        in.schemeName = scheme;
        in.windowBg = windowScheme.background(KColorScheme::NormalBackground).color();
        in.windowAltBg = windowScheme.background(KColorScheme::AlternateBackground).color();
        in.windowFg = windowScheme.foreground(KColorScheme::NormalText).color();
        in.windowInactiveFg = inactiveWindow.foreground(KColorScheme::InactiveText).color();
        in.viewBg = viewScheme.background(KColorScheme::NormalBackground).color();
        in.viewAltBg = viewScheme.background(KColorScheme::AlternateBackground).color();
        in.viewFg = viewScheme.foreground(KColorScheme::NormalText).color();
        in.viewInactiveFg = inactiveView.foreground(KColorScheme::InactiveText).color();
        in.buttonBg = buttonScheme.background(KColorScheme::NormalBackground).color();
        in.buttonFg = buttonScheme.foreground(KColorScheme::NormalText).color();
        in.selectionBg = selectionScheme.background(KColorScheme::NormalBackground).color();
        in.selectionFg = selectionScheme.foreground(KColorScheme::NormalText).color();
        in.focusColor = viewScheme.decoration(KColorScheme::FocusColor).color();
        in.hoverColor = viewScheme.decoration(KColorScheme::HoverColor).color();
        in.linkFg = viewScheme.foreground(KColorScheme::LinkText).color();
        in.positiveFg = viewScheme.foreground(KColorScheme::PositiveText).color();
        in.warningFg = viewScheme.foreground(KColorScheme::NeutralText).color();
        in.negativeFg = viewScheme.foreground(KColorScheme::NegativeText).color();

        if (in.windowBg.isValid() && in.viewBg.isValid()) {
            nextActive = true;
            nextPalette = derivePalette(in);
            nextAppPalette = KColorScheme::createApplicationPalette(m_kde->config);
        }
    }
#else
    if (auto *hints = QGuiApplication::styleHints())
        nextPalette.isDark = hints->colorScheme() == Qt::ColorScheme::Dark;
#endif

    if (m_active == nextActive && m_palette == nextPalette
        && m_applicationPalette == nextAppPalette) {
        return;
    }
    m_active = nextActive;
    m_palette = nextPalette;
    m_applicationPalette = nextAppPalette;
    Q_EMIT themeChanged();
}
