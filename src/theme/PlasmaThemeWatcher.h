#pragma once

#include <QColor>
#include <QObject>
#include <QPalette>
#include <QString>
#include <QTimer>

#include <memory>

class QFileSystemWatcher;

class PlasmaThemeWatcher : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool active READ active NOTIFY themeChanged)
    Q_PROPERTY(bool kdeIntegrationAvailable READ kdeIntegrationAvailable CONSTANT)
    Q_PROPERTY(bool isDark READ isDark NOTIFY themeChanged)
    Q_PROPERTY(QString schemeName READ schemeName NOTIFY themeChanged)

    Q_PROPERTY(QColor background READ background NOTIFY themeChanged)
    Q_PROPERTY(QColor rail READ rail NOTIFY themeChanged)
    Q_PROPERTY(QColor sidebar READ sidebar NOTIFY themeChanged)
    Q_PROPERTY(QColor surface READ surface NOTIFY themeChanged)
    Q_PROPERTY(QColor cardElevated READ cardElevated NOTIFY themeChanged)
    Q_PROPERTY(QColor hover READ hover NOTIFY themeChanged)
    Q_PROPERTY(QColor selected READ selected NOTIFY themeChanged)
    Q_PROPERTY(QColor selectedHover READ selectedHover NOTIFY themeChanged)
    Q_PROPERTY(QColor selectedText READ selectedText NOTIFY themeChanged)
    Q_PROPERTY(QColor inputBg READ inputBg NOTIFY themeChanged)
    Q_PROPERTY(QColor codeBlock READ codeBlock NOTIFY themeChanged)
    Q_PROPERTY(QColor textPrimary READ textPrimary NOTIFY themeChanged)
    Q_PROPERTY(QColor textSecondary READ textSecondary NOTIFY themeChanged)
    Q_PROPERTY(QColor textMuted READ textMuted NOTIFY themeChanged)
    Q_PROPERTY(QColor textDisabled READ textDisabled NOTIFY themeChanged)
    Q_PROPERTY(QColor border READ border NOTIFY themeChanged)
    Q_PROPERTY(QColor borderStrong READ borderStrong NOTIFY themeChanged)
    Q_PROPERTY(QColor accent READ accent NOTIFY themeChanged)
    Q_PROPERTY(QColor accentHover READ accentHover NOTIFY themeChanged)
    Q_PROPERTY(QColor accentPressed READ accentPressed NOTIFY themeChanged)
    Q_PROPERTY(QColor accentSoft READ accentSoft NOTIFY themeChanged)
    Q_PROPERTY(QColor accentBorder READ accentBorder NOTIFY themeChanged)
    Q_PROPERTY(QColor accentText READ accentText NOTIFY themeChanged)
    Q_PROPERTY(QColor ownBubble READ ownBubble NOTIFY themeChanged)
    Q_PROPERTY(QColor ownBubbleText READ ownBubbleText NOTIFY themeChanged)
    Q_PROPERTY(QColor otherBubble READ otherBubble NOTIFY themeChanged)
    Q_PROPERTY(QColor reaction READ reaction NOTIFY themeChanged)
    Q_PROPERTY(QColor link READ link NOTIFY themeChanged)
    Q_PROPERTY(QColor positive READ positive NOTIFY themeChanged)
    Q_PROPERTY(QColor warning READ warning NOTIFY themeChanged)
    Q_PROPERTY(QColor negative READ negative NOTIFY themeChanged)
    Q_PROPERTY(QColor info READ info NOTIFY themeChanged)
    Q_PROPERTY(QColor mention READ mention NOTIFY themeChanged)
    Q_PROPERTY(QColor dangerTint READ dangerTint NOTIFY themeChanged)

public:
    struct PaletteInput {
        QString schemeName;
        QColor windowBg;
        QColor windowAltBg;
        QColor windowFg;
        QColor windowInactiveFg;
        QColor viewBg;
        QColor viewAltBg;
        QColor viewFg;
        QColor viewInactiveFg;
        QColor buttonBg;
        QColor buttonFg;
        QColor selectionBg;
        QColor selectionFg;
        QColor focusColor;
        QColor hoverColor;
        QColor linkFg;
        QColor positiveFg;
        QColor warningFg;
        QColor negativeFg;
    };

    struct DerivedPalette {
        bool isDark = false;
        QString schemeName;
        QColor background;
        QColor rail;
        QColor sidebar;
        QColor surface;
        QColor cardElevated;
        QColor hover;
        QColor selected;
        QColor selectedHover;
        QColor selectedText;
        QColor inputBg;
        QColor codeBlock;
        QColor textPrimary;
        QColor textSecondary;
        QColor textMuted;
        QColor textDisabled;
        QColor border;
        QColor borderStrong;
        QColor accent;
        QColor accentHover;
        QColor accentPressed;
        QColor accentSoft;
        QColor accentBorder;
        QColor accentText;
        QColor ownBubble;
        QColor ownBubbleText;
        QColor otherBubble;
        QColor reaction;
        QColor link;
        QColor positive;
        QColor warning;
        QColor negative;
        QColor info;
        QColor mention;
        QColor dangerTint;

        bool operator==(const DerivedPalette &) const = default;
    };

    static DerivedPalette derivePalette(const PaletteInput &in);

    explicit PlasmaThemeWatcher(QObject *parent = nullptr);
    ~PlasmaThemeWatcher() override;

    bool active() const { return m_active; }
    bool kdeIntegrationAvailable() const;
    bool isDark() const { return m_palette.isDark; }
    QString schemeName() const { return m_palette.schemeName; }

    QColor background() const { return m_palette.background; }
    QColor rail() const { return m_palette.rail; }
    QColor sidebar() const { return m_palette.sidebar; }
    QColor surface() const { return m_palette.surface; }
    QColor cardElevated() const { return m_palette.cardElevated; }
    QColor hover() const { return m_palette.hover; }
    QColor selected() const { return m_palette.selected; }
    QColor selectedHover() const { return m_palette.selectedHover; }
    QColor selectedText() const { return m_palette.selectedText; }
    QColor inputBg() const { return m_palette.inputBg; }
    QColor codeBlock() const { return m_palette.codeBlock; }
    QColor textPrimary() const { return m_palette.textPrimary; }
    QColor textSecondary() const { return m_palette.textSecondary; }
    QColor textMuted() const { return m_palette.textMuted; }
    QColor textDisabled() const { return m_palette.textDisabled; }
    QColor border() const { return m_palette.border; }
    QColor borderStrong() const { return m_palette.borderStrong; }
    QColor accent() const { return m_palette.accent; }
    QColor accentHover() const { return m_palette.accentHover; }
    QColor accentPressed() const { return m_palette.accentPressed; }
    QColor accentSoft() const { return m_palette.accentSoft; }
    QColor accentBorder() const { return m_palette.accentBorder; }
    QColor accentText() const { return m_palette.accentText; }
    QColor ownBubble() const { return m_palette.ownBubble; }
    QColor ownBubbleText() const { return m_palette.ownBubbleText; }
    QColor otherBubble() const { return m_palette.otherBubble; }
    QColor reaction() const { return m_palette.reaction; }
    QColor link() const { return m_palette.link; }
    QColor positive() const { return m_palette.positive; }
    QColor warning() const { return m_palette.warning; }
    QColor negative() const { return m_palette.negative; }
    QColor info() const { return m_palette.info; }
    QColor mention() const { return m_palette.mention; }
    QColor dangerTint() const { return m_palette.dangerTint; }

    QPalette applicationPalette() const { return m_applicationPalette; }
    void applyApplicationPalette();

    Q_INVOKABLE void refresh();

    void applyTestPaletteInput(const PaletteInput &in, bool active = true);

Q_SIGNALS:
    void themeChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void scheduleRefresh();
    void reloadFromDesktop();
    void ensureKdeGlobalsWatched();

    bool m_active = false;
    bool m_applyingPalette = false;
    bool m_testOverride = false;
    DerivedPalette m_palette;
    QPalette m_applicationPalette;
    QTimer m_refreshTimer;
    QString m_kdeGlobalsPath;
    QFileSystemWatcher *m_fileWatcher = nullptr;
    struct KdeImpl;
    std::unique_ptr<KdeImpl> m_kde;
};
