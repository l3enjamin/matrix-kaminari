#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class BackgroundPortal : public QObject
{
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool active READ active NOTIFY stateChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(bool portalAvailable READ portalAvailable NOTIFY stateChanged)
    Q_PROPERTY(QString backendName READ backendName NOTIFY stateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)
    Q_PROPERTY(QString autostartFilePath READ autostartFilePath
                   NOTIFY stateChanged)

public:
    enum State {
        Disabled = 0,
        Checking = 1,
        EnabledPortal = 2,
        EnabledDesktopFile = 3,
        Denied = 4,
        Unavailable = 5,
    };
    Q_ENUM(State)

    static QString desktopFileId();
    static QString resolveAutostartFilePath();
    static QStringList autostartCommandArguments();
    static QString buildAutostartDesktopEntry(const QString &execPath,
                                              const QString &appId,
                                              const QString &displayName,
                                              const QStringList &arguments = autostartCommandArguments());

    explicit BackgroundPortal(QObject *parent = nullptr);
    ~BackgroundPortal() override;

    State state() const { return m_state; }
    bool active() const
    {
        return m_state == EnabledPortal || m_state == EnabledDesktopFile;
    }
    bool busy() const { return m_busy || m_state == Checking; }
    bool portalAvailable() const { return m_portalAvailable; }
    QString backendName() const;
    QString statusText() const;
    QString autostartFilePath() const;

    void setPreferPortalForTest(int mode) { m_preferPortalForTest = mode; }
    void setExecPathForTest(const QString &path) { m_execPathForTest = path; }

    void syncInitial(bool enabled);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setAutostart(bool enabled);

Q_SIGNALS:
    void stateChanged();

private:
    void setState(State state, const QString &detail = QString());
    void setBusy(bool busy);
    void setPortalAvailable(bool available);
    QString resolvedExecPath() const;
    bool applyDesktopEntry(bool enable, QString *errorOut);
    void requestPortalBackground(bool enable, int generation);
    void sendBackgroundRequest(bool enable, int generation);

    State m_state = Disabled;
    bool m_busy = false;
    bool m_portalAvailable = false;
    bool m_portalGrantedThisSession = false;
    bool m_desiredEnabled = false;
    QString m_detail;
    int m_generation = 0;
    // -1: auto (portal then desktop entry fallback), 0: desktop file only,
    // 1: portal only.
    int m_preferPortalForTest = -1;
    QString m_execPathForTest;
};
