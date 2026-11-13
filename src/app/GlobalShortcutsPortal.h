#pragma once

#include <QHash>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#ifdef HAVE_QT_DBUS
#include <QDBusObjectPath>
#endif

class QAction;
class ShortcutRegistry;
#ifdef HAVE_QT_DBUS
class QDBusServiceWatcher;
#endif

class GlobalShortcutsPortal : public QObject
{
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)
    Q_PROPERTY(bool canConfigure READ canConfigure NOTIFY stateChanged)
    Q_PROPERTY(QString backendName READ backendName NOTIFY stateChanged)
    Q_PROPERTY(QVariantList activeBindings READ activeBindings NOTIFY stateChanged)

public:
    enum State {
        Off = 0,
        Unavailable = 1,
        Connecting = 2,
        Active = 3,
        Failed = 4,
    };
    Q_ENUM(State)

    static QStringList actionIds();
    static QStringList kdeActionIds();

    static QString portalTrigger(const QString &portableText);

    explicit GlobalShortcutsPortal(ShortcutRegistry *registry,
                                   QObject *parent = nullptr);
    ~GlobalShortcutsPortal() override;

    State state() const { return m_state; }
    QString statusText() const;
    bool canConfigure() const;
    bool enabled() const { return m_enabled; }
    QString backendName() const;
    QVariantList activeBindings() const;

    void setEnabled(bool enabled);
    void setKdeBackendForcedForTest(int mode) { m_kdeBackendForced = mode; }

    Q_INVOKABLE void configure();

Q_SIGNALS:
    void stateChanged();
    void activated(const QString &actionId);
    void deactivated(const QString &actionId);

#ifdef HAVE_QT_DBUS
private Q_SLOTS:
    void onPortalActivated(const QDBusObjectPath &session,
                           const QString &shortcutId, qulonglong timestamp,
                           const QVariantMap &options);
    void onPortalDeactivated(const QDBusObjectPath &session,
                             const QString &shortcutId, qulonglong timestamp,
                             const QVariantMap &options);
    void onSessionClosed(const QVariantMap &details);
    void onPortalOwnerChanged(const QString &service, const QString &oldOwner,
                              const QString &newOwner);
#endif

private:
    bool useKdeBackend() const;
    void setupKdeActions();
    void teardownKdeActions(bool forgetShortcuts);
    void refreshKdeActionTexts();
    void start();
    void stop();
    void teardown();
    void beginSession();
    void setState(State state, const QString &detail = QString());
    void probeVersion(int generation);
    void registerApp(int generation);
    void createSession(int generation);
    void bindShortcuts(int generation);
    void closeSession(const QString &handle);
    void subscribeSession();
    void unsubscribeSession();

    ShortcutRegistry *m_registry = nullptr;
    bool m_enabled = false;
    State m_state = Off;
    QString m_detail;
    int m_version = 0;
    QString m_sessionHandle;
    int m_generation = 0;
    bool m_subscribed = false;
    bool m_usingKde = false;
    int m_kdeBackendForced = -1;
    QHash<QString, QString> m_portalTriggers;
#ifdef KAMINARI_ENABLE_KDE
    QHash<QString, QAction *> m_kdeActions;
    QMetaObject::Connection m_activeChangedConn;
    QMetaObject::Connection m_shortcutChangedConn;
#endif
#ifdef HAVE_QT_DBUS
    /// Owner changes of the portal's bus name: a restarted portal sends no
    /// Session.Closed, so this is the only way to learn the session is gone.
    QDBusServiceWatcher *m_watcher = nullptr;
#endif
};
