#include "app/BackgroundPortal.h"

#include "app/DesktopEntryQuoting.h"
#include "app/SandboxEnvironment.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QRandomGenerator>
#include <QSaveFile>
#include <QStandardPaths>

#if defined(HAVE_QT_DBUS) && !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)
#include "calls/PortalRequest.h"

#include <QDBusConnection>
#include <QDBusError>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCall>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QVariantMap>
#endif

namespace {
Q_LOGGING_CATEGORY(lcBackgroundPortal, "lightning.app.backgroundportal")

#if defined(HAVE_QT_DBUS) && !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)
constexpr auto kService = "org.freedesktop.portal.Desktop";
constexpr auto kPath = "/org/freedesktop/portal/desktop";
constexpr auto kBackground = "org.freedesktop.portal.Background";
constexpr auto kRequest = "org.freedesktop.portal.Request";
constexpr auto kHostRegistry = "org.freedesktop.host.portal.Registry";
constexpr int kCallTimeoutMs = 10000;

QString freshToken()
{
    return QStringLiteral("kaminari_bg_%1")
        .arg(QRandomGenerator::system()->generate64(), 0, 16);
}
#endif

bool containsControlChars(const QString &text)
{
    for (const QChar c : text) {
        if (c.category() == QChar::Other_Control)
            return true;
    }
    return false;
}

bool isSimpleBareArg(const QString &arg)
{
    if (arg.isEmpty())
        return false;
    for (const QChar c : arg) {
        if (c.isLetterOrNumber() || c == QLatin1Char('-')
            || c == QLatin1Char('_') || c == QLatin1Char('.')
            || c == QLatin1Char('/') || c == QLatin1Char('=')
            || c == QLatin1Char(':'))
            continue;
        return false;
    }
    return true;
}

bool readDesktopEntryFile(const QString &path, QByteArray *bytesOut,
                          QString *errorOut)
{
    const QFileInfo info(path);
    if (info.isSymLink()) {
        if (errorOut) {
            *errorOut = QCoreApplication::translate(
                "BackgroundPortal",
                "Existing autostart entry is a symlink; refusing to modify it.");
        }
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorOut) {
            *errorOut = QCoreApplication::translate(
                "BackgroundPortal",
                "Could not read the existing autostart entry.");
        }
        return false;
    }
    *bytesOut = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        if (errorOut) {
            *errorOut = QCoreApplication::translate(
                "BackgroundPortal",
                "Could not read the existing autostart entry.");
        }
        return false;
    }
    return true;
}

bool desktopEntryIsEnabled(const QByteArray &raw)
{
    if (raw.isEmpty())
        return false;
    bool inDesktopEntry = false;
    bool hasExec = false;
    bool hidden = false;
    bool gnomeAutostartEnabled = true;

    const QStringList lines = QString::fromUtf8(raw).split(QLatin1Char('\n'));
    for (const QString &rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;
        if (line.startsWith(QLatin1Char('['))) {
            if (inDesktopEntry)
                break;
            inDesktopEntry = (line == QLatin1String("[Desktop Entry]"));
            continue;
        }
        if (!inDesktopEntry)
            continue;
        const qsizetype eq = line.indexOf(QLatin1Char('='));
        if (eq <= 0)
            continue;
        const QString key = line.left(eq).trimmed();
        const QString val = line.mid(eq + 1).trimmed();
        if (key == QLatin1String("Exec") && !val.isEmpty()) {
            hasExec = true;
        } else if (key == QLatin1String("Hidden")) {
            hidden = (val.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0);
        } else if (key == QLatin1String("X-GNOME-Autostart-enabled")) {
            gnomeAutostartEnabled =
                (val.compare(QLatin1String("false"), Qt::CaseInsensitive) != 0);
        }
    }
    return hasExec && !hidden && gnomeAutostartEnabled;
}

} // namespace

#if defined(HAVE_QT_DBUS) && !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)
class BackgroundPortalStep : public QObject
{
    Q_OBJECT
public:
    BackgroundPortalStep(QObject *owner, const QString &requestPath)
        : QObject(owner), m_path(requestPath)
    {
        QDBusConnection::sessionBus().connect(
            QString(), m_path, QLatin1String(kRequest),
            QStringLiteral("Response"), this,
            SLOT(onResponse(uint, QVariantMap)));
    }
    ~BackgroundPortalStep() override
    {
        QDBusConnection::sessionBus().disconnect(
            QString(), m_path, QLatin1String(kRequest),
            QStringLiteral("Response"), this,
            SLOT(onResponse(uint, QVariantMap)));
    }

Q_SIGNALS:
    void answered(uint response, const QVariantMap &results);

private Q_SLOTS:
    void onResponse(uint response, const QVariantMap &results)
    {
        Q_EMIT answered(response, results);
        deleteLater();
    }

private:
    QString m_path;
};
#endif

QString BackgroundPortal::desktopFileId()
{
    QString id = QGuiApplication::desktopFileName().trimmed();
    if (id.endsWith(QLatin1String(".desktop")))
        id.chop(8);
    if (id.isEmpty()) {
        id = qEnvironmentVariable("FLATPAK_ID").trimmed();
        if (id.endsWith(QLatin1String(".desktop")))
            id.chop(8);
    }
    if (!id.isEmpty())
        return id;
#ifdef KAMINARI_BRANDING
    return QStringLiteral("kaminari");
#else
    return QStringLiteral("lightning");
#endif
}

QString BackgroundPortal::resolveAutostartFilePath()
{
    QString configHome = qEnvironmentVariable("XDG_CONFIG_HOME");
    if (configHome.isEmpty()) {
        configHome = QStandardPaths::writableLocation(
            QStandardPaths::GenericConfigLocation);
    }
    if (configHome.isEmpty())
        return {};
    return QDir(configHome)
        .filePath(QStringLiteral("autostart/") + desktopFileId()
                  + QStringLiteral(".desktop"));
}

QStringList BackgroundPortal::autostartCommandArguments()
{
    return { QStringLiteral("--minimized") };
}

QString BackgroundPortal::buildAutostartDesktopEntry(
    const QString &execPath, const QString &appId, const QString &displayName,
    const QStringList &arguments)
{
    if (execPath.trimmed().isEmpty() || appId.trimmed().isEmpty()
        || displayName.trimmed().isEmpty() || containsControlChars(appId)
        || containsControlChars(displayName))
        return {};

    const QString quotedExec =
        lightning::desktop_entry::quoteExecArgument(execPath);
    if (quotedExec.isEmpty())
        return {};

    QString execLine = quotedExec;
    for (const QString &arg : arguments) {
        if (arg.isEmpty() || containsControlChars(arg))
            return {};
        execLine += QLatin1Char(' ');
        if (isSimpleBareArg(arg)) {
            execLine += arg;
        } else {
            const QString quotedArg =
                lightning::desktop_entry::quoteExecArgument(arg);
            if (quotedArg.isEmpty())
                return {};
            execLine += quotedArg;
        }
    }

    QStringList lines;
    lines << QStringLiteral("[Desktop Entry]")
          << QStringLiteral("Type=Application")
          << QStringLiteral("Name=") + displayName
          << QStringLiteral("GenericName=Matrix Client")
          << QStringLiteral("Comment=Chat and calls on Matrix")
          << QStringLiteral("Exec=") + execLine;
    if (!execPath.contains(QLatin1Char(' '))
        && !execPath.contains(QLatin1Char('\t'))) {
        QString tryExec = execPath;
        tryExec.replace(QLatin1Char('\\'), QLatin1String("\\\\"));
        lines << QStringLiteral("TryExec=") + tryExec;
    }
    lines << QStringLiteral("Icon=") + appId
          << QStringLiteral("Terminal=false")
          << QStringLiteral("Categories=Network;Chat;InstantMessaging;")
          << QStringLiteral("X-GNOME-Autostart-enabled=true");

    return lightning::desktop_entry::withGeneratedMarker(
        lines.join(QLatin1Char('\n')) + QLatin1Char('\n'));
}

BackgroundPortal::BackgroundPortal(QObject *parent)
    : QObject(parent)
{
}

BackgroundPortal::~BackgroundPortal()
{
    ++m_generation;
}

QString BackgroundPortal::backendName() const
{
    switch (m_state) {
    case Disabled:
        return QStringLiteral("off");
    case Checking:
        return QStringLiteral("checking");
    case EnabledPortal:
        return QStringLiteral("portal");
    case EnabledDesktopFile:
        return QStringLiteral("autostart-desktop");
    case Denied:
        return QStringLiteral("denied");
    case Unavailable:
        return QStringLiteral("unavailable");
    }
    return QStringLiteral("off");
}

QString BackgroundPortal::statusText() const
{
    switch (m_state) {
    case Disabled:
        return tr("Off. Application will not start automatically at login.");
    case Checking:
        return tr("Checking desktop autostart permission...");
    case EnabledPortal:
        return tr("On (XDG Background portal). Launches minimized at login.");
    case EnabledDesktopFile:
        return tr("On (autostart desktop entry). Launches minimized at login.");
    case Denied:
        return m_detail.isEmpty()
            ? tr("Autostart permission was denied by your desktop.")
            : m_detail;
    case Unavailable:
        return m_detail.isEmpty()
            ? tr("Autostart is unavailable in this environment.")
            : m_detail;
    }
    return {};
}

QString BackgroundPortal::autostartFilePath() const
{
    return resolveAutostartFilePath();
}

void BackgroundPortal::setState(State state, const QString &detail)
{
    if (m_state == state && m_detail == detail)
        return;
    m_state = state;
    m_detail = detail;
    Q_EMIT stateChanged();
}

void BackgroundPortal::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    Q_EMIT stateChanged();
}

void BackgroundPortal::setPortalAvailable(bool available)
{
    if (m_portalAvailable == available)
        return;
    m_portalAvailable = available;
    Q_EMIT stateChanged();
}

QString BackgroundPortal::resolvedExecPath() const
{
    if (!m_execPathForTest.isEmpty())
        return m_execPathForTest;
    const QString appImage = qEnvironmentVariable("APPIMAGE").trimmed();
    if (!appImage.isEmpty())
        return appImage;
    return QCoreApplication::applicationFilePath();
}

bool BackgroundPortal::applyDesktopEntry(bool enable, QString *errorOut)
{
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
    if (errorOut) {
        *errorOut = tr("Desktop autostart entries are only used on Linux.");
    }
    Q_UNUSED(enable);
    return false;
#else
    if (sandboxenv::isSandboxed()) {
        if (errorOut) {
            *errorOut = tr(
                "Sandboxed apps cannot manage ~/.config/autostart directly.");
        }
        return false;
    }

    const QString path = resolveAutostartFilePath();
    if (path.isEmpty()) {
        if (errorOut) {
            *errorOut = tr("No writable XDG config directory is available.");
        }
        return false;
    }

    const QFileInfo entryInfo(path);
    const QFileInfo autostartDirInfo(entryInfo.absolutePath());
    if (autostartDirInfo.isSymLink()) {
        if (errorOut) {
            *errorOut = tr("Autostart directory is a symlink; refusing to "
                           "modify it.");
        }
        return false;
    }
    if (entryInfo.isSymLink()) {
        if (errorOut) {
            *errorOut = tr("Existing autostart entry is a symlink; refusing "
                           "to modify it.");
        }
        return false;
    }

    const bool exists = entryInfo.exists();
    QByteArray existingBytes;
    if (exists && !readDesktopEntryFile(path, &existingBytes, errorOut))
        return false;

    if (!enable) {
        if (!exists)
            return true;
        const QString existingText = QString::fromUtf8(existingBytes);
        if (!lightning::desktop_entry::isUntouchedGeneratedEntry(existingText)) {
            if (errorOut) {
                *errorOut = tr("Existing autostart entry was customized outside "
                               "the application and was left untouched.");
            }
            return false;
        }
        if (!QFile::remove(path)) {
            if (errorOut) {
                *errorOut = tr("Could not remove the autostart entry at %1.")
                                .arg(path);
            }
            return false;
        }
        return true;
    }

    if (exists) {
        const QString existingText = QString::fromUtf8(existingBytes);
        if (!lightning::desktop_entry::isUntouchedGeneratedEntry(existingText)) {
            if (desktopEntryIsEnabled(existingBytes))
                return true;
            if (errorOut) {
                *errorOut = tr("Existing autostart entry was customized outside "
                               "the application and was left untouched.");
            }
            return false;
        }
    }

    QString displayName = QGuiApplication::applicationDisplayName().trimmed();
    if (displayName.isEmpty()) {
#ifdef KAMINARI_BRANDING
        displayName = QStringLiteral("Kaminari");
#else
        displayName = QStringLiteral("Lightning");
#endif
    }

    const QByteArray wanted = buildAutostartDesktopEntry(
        resolvedExecPath(), desktopFileId(), displayName,
        autostartCommandArguments()).toUtf8();
    if (wanted.isEmpty()) {
        if (errorOut) {
            *errorOut = tr("Could not build a valid autostart entry for the "
                           "executable path.");
        }
        return false;
    }

    if (exists && existingBytes == wanted)
        return true;

    if (!QDir().mkpath(entryInfo.absolutePath())) {
        if (errorOut) {
            *errorOut = tr("Could not create the autostart directory %1.")
                            .arg(entryInfo.absolutePath());
        }
        return false;
    }

    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)
        || out.write(wanted) != wanted.size() || !out.commit()) {
        if (errorOut) {
            *errorOut = tr("Could not write the autostart entry at %1.")
                            .arg(path);
        }
        return false;
    }
    return true;
#endif
}

void BackgroundPortal::syncInitial(bool enabled)
{
    m_desiredEnabled = enabled;
    const QString path = resolveAutostartFilePath();
    QByteArray existing;
    QString err;
    if (!path.isEmpty() && QFileInfo::exists(path)
        && !QFileInfo(path).isSymLink()
        && readDesktopEntryFile(path, &existing, &err)
        && desktopEntryIsEnabled(existing)) {
        setState(EnabledDesktopFile);
        return;
    }
    if (!enabled) {
        setState(Disabled);
        return;
    }
    if (m_preferPortalForTest == 0
        && !sandboxenv::isSandboxed()) {
        QString writeErr;
        if (applyDesktopEntry(true, &writeErr))
            setState(EnabledDesktopFile);
        else
            setState(Unavailable, writeErr);
        return;
    }
    if (m_portalGrantedThisSession) {
        setState(EnabledPortal);
        return;
    }
    setState(Disabled);
}

void BackgroundPortal::refresh()
{
    const QString path = resolveAutostartFilePath();
    if (!path.isEmpty() && QFileInfo::exists(path)) {
        QByteArray existing;
        QString err;
        if (QFileInfo(path).isSymLink()) {
            setState(Unavailable,
                     tr("Existing autostart entry is a symlink; refusing to "
                        "modify it."));
        } else if (readDesktopEntryFile(path, &existing, &err)
                   && desktopEntryIsEnabled(existing)) {
            setState(EnabledDesktopFile);
        } else if (m_portalGrantedThisSession) {
            setState(EnabledPortal);
        } else {
            setState(Disabled);
        }
    } else if (m_portalGrantedThisSession) {
        setState(EnabledPortal);
    } else if (m_state != Denied && m_state != Checking) {
        setState(Disabled);
    }

#if defined(HAVE_QT_DBUS) && !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)
    if (!QDBusConnection::sessionBus().isConnected()) {
        setPortalAvailable(false);
        return;
    }
    const int gen = m_generation;
    QDBusMessage probe = QDBusMessage::createMethodCall(
        QLatin1String(kService), QLatin1String(kPath),
        QStringLiteral("org.freedesktop.DBus.Properties"),
        QStringLiteral("Get"));
    probe << QLatin1String(kBackground) << QStringLiteral("version");
    auto *watcher = new QDBusPendingCallWatcher(
        QDBusConnection::sessionBus().asyncCall(probe, kCallTimeoutMs), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, gen](QDBusPendingCallWatcher *w) {
                w->deleteLater();
                if (gen != m_generation)
                    return;
                const QDBusMessage reply = w->reply();
                const bool ok = reply.type() == QDBusMessage::ReplyMessage
                    && !reply.arguments().isEmpty();
                setPortalAvailable(ok);
            });
#else
    setPortalAvailable(false);
#endif
}

void BackgroundPortal::setAutostart(bool enabled)
{
    m_desiredEnabled = enabled;
    const int generation = ++m_generation;

    if (m_preferPortalForTest == 0) {
        setBusy(false);
        m_portalGrantedThisSession = false;
        QString error;
        if (applyDesktopEntry(enabled, &error))
            setState(enabled ? EnabledDesktopFile : Disabled);
        else
            setState(Unavailable, error);
        return;
    }

#if defined(HAVE_QT_DBUS) && !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)
    if (QDBusConnection::sessionBus().isConnected()) {
        setBusy(true);
        setState(Checking);
        requestPortalBackground(enabled, generation);
        return;
    }
#endif

    if (m_preferPortalForTest == 1 || sandboxenv::isSandboxed()) {
        setBusy(false);
        setState(Unavailable,
                 tr("xdg-desktop-portal Background service is unavailable."));
        return;
    }

    setBusy(false);
    m_portalGrantedThisSession = false;
    QString error;
    if (applyDesktopEntry(enabled, &error))
        setState(enabled ? EnabledDesktopFile : Disabled);
    else
        setState(Unavailable, error);
}

void BackgroundPortal::requestPortalBackground(bool enable, int generation)
{
#if defined(HAVE_QT_DBUS) && !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)
    const QString appId = desktopFileId();
    if (appId.isEmpty() || sandboxenv::isFlatpak()) {
        sendBackgroundRequest(enable, generation);
        return;
    }
    QDBusMessage reg = QDBusMessage::createMethodCall(
        QLatin1String(kService), QLatin1String(kPath),
        QLatin1String(kHostRegistry), QStringLiteral("Register"));
    reg << appId << QVariantMap{};
    auto *watcher = new QDBusPendingCallWatcher(
        QDBusConnection::sessionBus().asyncCall(reg, kCallTimeoutMs), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, enable, generation](QDBusPendingCallWatcher *w) {
                w->deleteLater();
                if (w->isError()) {
                    qCInfo(lcBackgroundPortal)
                        << "host portal registry skipped:" << w->error().name();
                }
                if (generation == m_generation)
                    sendBackgroundRequest(enable, generation);
            });
#else
    Q_UNUSED(enable);
    Q_UNUSED(generation);
#endif
}

void BackgroundPortal::sendBackgroundRequest(bool enable, int generation)
{
#if defined(HAVE_QT_DBUS) && !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)
    const QString token = freshToken();
    const auto fallbackOrFail = [this, enable](const QString &portalReason) {
        if (m_preferPortalForTest == 1 || sandboxenv::isSandboxed()) {
            setBusy(false);
            setState(Unavailable, portalReason);
            return;
        }
        qCInfo(lcBackgroundPortal)
            << "falling back to XDG autostart desktop entry:" << portalReason;
        QString desktopErr;
        setBusy(false);
        if (applyDesktopEntry(enable, &desktopErr))
            setState(enable ? EnabledDesktopFile : Disabled);
        else
            setState(Unavailable, desktopErr);
    };

    const auto onAnswer = [this, enable, generation, fallbackOrFail](
                              uint response, const QVariantMap &results) {
        if (generation != m_generation)
            return;
        if (response == 0) {
            setPortalAvailable(true);
            const bool grantedAutostart =
                results.contains(QStringLiteral("autostart"))
                    ? results.value(QStringLiteral("autostart")).toBool()
                    : enable;
            if (enable) {
                setBusy(false);
                if (grantedAutostart) {
                    m_portalGrantedThisSession = true;
                    setState(EnabledPortal);
                } else {
                    m_portalGrantedThisSession = false;
                    setState(Denied,
                             tr("Your desktop allowed background execution "
                                "but declined autostart at login."));
                }
                return;
            }
            m_portalGrantedThisSession = false;
            setBusy(false);
            const QString path = resolveAutostartFilePath();
            if (!path.isEmpty() && QFileInfo::exists(path)) {
                QString removeErr;
                applyDesktopEntry(false, &removeErr);
            }
            setState(Disabled);
            return;
        }
        if (response == 1) {
            setBusy(false);
            m_portalGrantedThisSession = false;
            if (!enable) {
                const QString path = resolveAutostartFilePath();
                if (!path.isEmpty() && QFileInfo::exists(path)) {
                    QString removeErr;
                    if (applyDesktopEntry(false, &removeErr)) {
                        setState(Disabled);
                        return;
                    }
                }
            }
            setState(Denied,
                     tr("You declined autostart permission in the desktop "
                        "prompt."));
            return;
        }
        m_portalGrantedThisSession = false;
        fallbackOrFail(tr("Your desktop background portal returned an error "
                          "(code %1).").arg(response));
    };

    auto sub = portal::subscribeBeforeCall<BackgroundPortalStep>(
        this, QDBusConnection::sessionBus().baseService(), token, onAnswer);

    QStringList commandLine;
    const QString exec = resolvedExecPath();
    if (sandboxenv::isFlatpak()) {
        const QString bin = QFileInfo(exec).fileName();
        commandLine << (bin.isEmpty() ? desktopFileId() : bin);
    } else {
        commandLine << (exec.isEmpty() ? desktopFileId() : exec);
    }
    commandLine.append(autostartCommandArguments());

    QVariantMap options{
        { QStringLiteral("handle_token"), token },
        { QStringLiteral("reason"),
          tr("Launch at login to stay connected and deliver notifications.") },
        { QStringLiteral("autostart"), enable },
        { QStringLiteral("dbus-activatable"), false },
    };
    if (enable) {
        options.insert(QStringLiteral("background"), true);
        options.insert(QStringLiteral("commandline"), commandLine);
    }

    QDBusMessage call = QDBusMessage::createMethodCall(
        QLatin1String(kService), QLatin1String(kPath),
        QLatin1String(kBackground), QStringLiteral("RequestBackground"));
    call << QString() << options;

    auto *watcher = new QDBusPendingCallWatcher(
        QDBusConnection::sessionBus().asyncCall(call, kCallTimeoutMs), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, generation, sub, onAnswer, fallbackOrFail](
                QDBusPendingCallWatcher *w) mutable {
                w->deleteLater();
                QDBusPendingReply<QDBusObjectPath> reply = *w;
                if (generation != m_generation) {
                    portal::dropSubscription(sub);
                    return;
                }
                if (reply.isError()) {
                    portal::dropSubscription(sub);
                    setPortalAvailable(false);
                    qCInfo(lcBackgroundPortal)
                        << "RequestBackground failed:" << reply.error().name()
                        << reply.error().message();
                    fallbackOrFail(
                        tr("xdg-desktop-portal Background request failed (%1).")
                            .arg(reply.error().name()));
                    return;
                }
                setPortalAvailable(true);
                portal::reconcileRequestPath(this, sub, reply.value().path(),
                                             onAnswer);
            });
#else
    Q_UNUSED(enable);
    Q_UNUSED(generation);
#endif
}

#if defined(HAVE_QT_DBUS) && !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)
#include "BackgroundPortal.moc"
#endif
