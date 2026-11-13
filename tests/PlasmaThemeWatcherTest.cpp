#include <QColor>
#include <QtTest/QtTest>

#include "theme/PlasmaThemeWatcher.h"

class PlasmaThemeWatcherTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void derivesDarkPaletteFromBreezeDarkInputs()
    {
        PlasmaThemeWatcher::PaletteInput in;
        in.schemeName = QStringLiteral("BreezeDark");
        in.windowBg = QColor(QStringLiteral("#2a2e32"));
        in.windowAltBg = QColor(QStringLiteral("#31363b"));
        in.windowFg = QColor(QStringLiteral("#eff0f1"));
        in.windowInactiveFg = QColor(QStringLiteral("#bdc3c7"));
        in.viewBg = QColor(QStringLiteral("#1b1e20"));
        in.viewAltBg = QColor(QStringLiteral("#232629"));
        in.viewFg = QColor(QStringLiteral("#fcfcfc"));
        in.viewInactiveFg = QColor(QStringLiteral("#a1a9b1"));
        in.buttonBg = QColor(QStringLiteral("#31363b"));
        in.buttonFg = QColor(QStringLiteral("#eff0f1"));
        in.selectionBg = QColor(QStringLiteral("#3daee9"));
        in.selectionFg = QColor(QStringLiteral("#ffffff"));
        in.focusColor = QColor(QStringLiteral("#3daee9"));
        in.hoverColor = QColor(QStringLiteral("#93cee9"));
        in.linkFg = QColor(QStringLiteral("#2980b9"));
        in.positiveFg = QColor(QStringLiteral("#27ae60"));
        in.warningFg = QColor(QStringLiteral("#f67400"));
        in.negativeFg = QColor(QStringLiteral("#da4453"));

        const auto p = PlasmaThemeWatcher::derivePalette(in);
        QVERIFY(p.isDark);
        QCOMPARE(p.schemeName, QStringLiteral("BreezeDark"));
        QCOMPARE(p.accent, QColor(QStringLiteral("#3daee9")));
        QCOMPARE(p.positive, QColor(QStringLiteral("#27ae60")));
        QCOMPARE(p.warning, QColor(QStringLiteral("#f67400")));
        QCOMPARE(p.negative, QColor(QStringLiteral("#da4453")));
        QVERIFY(p.background.isValid());
        QVERIFY(p.surface.isValid());
        QVERIFY(p.textPrimary.isValid());
        QVERIFY(p.border.isValid());
        QVERIFY(p.ownBubble.isValid());
        QVERIFY(p.ownBubbleText.isValid());
    }

    void derivesLightPaletteFromBreezeLightInputs()
    {
        PlasmaThemeWatcher::PaletteInput in;
        in.schemeName = QStringLiteral("BreezeLight");
        in.windowBg = QColor(QStringLiteral("#eff0f1"));
        in.windowAltBg = QColor(QStringLiteral("#e3e5e7"));
        in.windowFg = QColor(QStringLiteral("#232629"));
        in.windowInactiveFg = QColor(QStringLiteral("#7f8c8d"));
        in.viewBg = QColor(QStringLiteral("#fcfcfc"));
        in.viewAltBg = QColor(QStringLiteral("#eff0f1"));
        in.viewFg = QColor(QStringLiteral("#232629"));
        in.viewInactiveFg = QColor(QStringLiteral("#7f8c8d"));
        in.buttonBg = QColor(QStringLiteral("#fcfcfc"));
        in.buttonFg = QColor(QStringLiteral("#232629"));
        in.selectionBg = QColor(QStringLiteral("#3daee9"));
        in.selectionFg = QColor(QStringLiteral("#ffffff"));
        in.focusColor = QColor(QStringLiteral("#3daee9"));
        in.hoverColor = QColor(QStringLiteral("#93cee9"));
        in.linkFg = QColor(QStringLiteral("#2980b9"));
        in.positiveFg = QColor(QStringLiteral("#27ae60"));
        in.warningFg = QColor(QStringLiteral("#f67400"));
        in.negativeFg = QColor(QStringLiteral("#da4453"));

        const auto p = PlasmaThemeWatcher::derivePalette(in);
        QVERIFY(!p.isDark);
        QCOMPARE(p.schemeName, QStringLiteral("BreezeLight"));
        QVERIFY(p.background.lightness() > 200);
        QVERIFY(p.textPrimary.lightness() < 80);
    }

    void watcherConstructsCleanlyInOffscreenEnv()
    {
        PlasmaThemeWatcher watcher;
        QCOMPARE(watcher.kdeIntegrationAvailable(),
#ifdef KAMINARI_ENABLE_KDE
                 true
#else
                 false
#endif
        );
    }
};

QTEST_MAIN(PlasmaThemeWatcherTest)
#include "PlasmaThemeWatcherTest.moc"
