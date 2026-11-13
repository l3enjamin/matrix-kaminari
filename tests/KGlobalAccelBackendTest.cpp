#include <QtTest/QtTest>

#include "app/GlobalShortcutsPortal.h"
#include "app/SettingsManager.h"
#include "app/ShortcutRegistry.h"

class KGlobalAccelBackendTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void actionIdsIncludeExpandedDesktopShortcuts()
    {
        const QStringList ids = GlobalShortcutsPortal::actionIds();
        QVERIFY(ids.contains(QStringLiteral("call.toggleMute")));
        QVERIFY(ids.contains(QStringLiteral("call.toggleDeafen")));
        QVERIFY(ids.contains(QStringLiteral("call.leave")));
        QVERIFY(ids.contains(QStringLiteral("call.pushToTalk")));
        QVERIFY(ids.contains(QStringLiteral("app.toggleWindow")));
        QVERIFY(ids.contains(QStringLiteral("nav.quickSwitcher")));
        QVERIFY(ids.contains(QStringLiteral("room.markRead")));
    }

    void disabledPortalReportsOffBackendAndEmptyBindings()
    {
        SettingsManager settings;
        ShortcutRegistry registry(&settings);
        GlobalShortcutsPortal portal(&registry);

        QCOMPARE(portal.state(), GlobalShortcutsPortal::Off);
        QCOMPARE(portal.backendName(), QStringLiteral("off"));
        QVERIFY(portal.activeBindings().isEmpty());
    }

    void forcedNonKdePortalWithoutDbusReportsUnavailable()
    {
        SettingsManager settings;
        ShortcutRegistry registry(&settings);
        GlobalShortcutsPortal portal(&registry);
        portal.setKdeBackendForcedForTest(0);
        portal.setEnabled(true);

        QVERIFY(portal.state() == GlobalShortcutsPortal::Unavailable
                || portal.state() == GlobalShortcutsPortal::Connecting
                || portal.state() == GlobalShortcutsPortal::Active);
    }

#ifdef KAMINARI_ENABLE_KDE
    void kdeBackendExposesActiveBindingsWhenEnabled()
    {
        SettingsManager settings;
        ShortcutRegistry registry(&settings);
        GlobalShortcutsPortal portal(&registry);
        portal.setKdeBackendForcedForTest(1);
        portal.setEnabled(true);

        QCOMPARE(portal.backendName(), QStringLiteral("kglobalaccel"));
        QCOMPARE(portal.state(), GlobalShortcutsPortal::Active);
        QVERIFY(portal.canConfigure());
        QCOMPARE(portal.activeBindings().size(),
                 GlobalShortcutsPortal::kdeActionIds().size());
    }
#endif
};

QTEST_MAIN(KGlobalAccelBackendTest)
#include "KGlobalAccelBackendTest.moc"
