#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "app/BackgroundPortal.h"

class BackgroundPortalTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void desktopEntryContainsMinimizedFlag()
    {
        const QString entry = BackgroundPortal::buildAutostartDesktopEntry(
            QStringLiteral("/usr/bin/kaminari"),
            QStringLiteral("kaminari"),
            QStringLiteral("Kaminari"));
        QVERIFY(entry.contains(QStringLiteral("[Desktop Entry]")));
        QVERIFY(entry.contains(QStringLiteral("Type=Application")));
        QVERIFY(entry.contains(QStringLiteral("Exec=/usr/bin/kaminari --minimized")));
        QVERIFY(entry.contains(QStringLiteral("Name=Kaminari")));
        QVERIFY(entry.contains(QStringLiteral("X-KDE-autostart-phase=2")));
    }

    void desktopFileFallbackWritesAndRemovesAutostartEntry()
    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        const QByteArray prevConfig = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", tempDir.path().toUtf8());

        BackgroundPortal portal;
        portal.setPreferPortalForTest(0);
        portal.setExecPathForTest(QStringLiteral("/usr/bin/kaminari"));

        portal.setAutostart(true);
        QCOMPARE(portal.state(), BackgroundPortal::EnabledDesktopFile);
        QVERIFY(portal.active());
        QCOMPARE(portal.backendName(), QStringLiteral("xdg-autostart"));

        const QString filePath = BackgroundPortal::resolveAutostartFilePath();
        QVERIFY(QFile::exists(filePath));
        QFile f(filePath);
        QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString written = QString::fromUtf8(f.readAll());
        QVERIFY(written.contains(QStringLiteral("--minimized")));

        portal.setAutostart(false);
        QCOMPARE(portal.state(), BackgroundPortal::Disabled);
        QVERIFY(!portal.active());
        QVERIFY(!QFile::exists(filePath));

        if (prevConfig.isEmpty())
            qunsetenv("XDG_CONFIG_HOME");
        else
            qputenv("XDG_CONFIG_HOME", prevConfig);
    }
};

QTEST_MAIN(BackgroundPortalTest)
#include "BackgroundPortalTest.moc"
