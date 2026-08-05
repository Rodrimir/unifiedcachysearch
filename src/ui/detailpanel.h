#ifndef DETAILPANEL_H
#define DETAILPANEL_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QStringList>
#include "core/searchitem.h"

class DetailPanel : public QWidget
{
    Q_OBJECT

public:
    explicit DetailPanel(QWidget *parent = nullptr);

    void showPackage(const SearchItem &item);
    void setSelectedCount(int count);
    void setUpdateMode(bool updateMode);
    QString currentCommand() const;
    void clear();
    void setThemeColors(bool dark);

signals:
    void installRequested(const QStringList &commands);
    void uninstallRequested(const QString &command);

private:
    void setupUI();

    QLabel *m_iconLabel;
    QLabel *m_nameLabel;
    QLabel *m_versionLabel;
    QLabel *m_repoLabel;
    QLabel *m_statusLabel;
    QLabel *m_descriptionLabel;
    QLabel *m_commandLabel;
    QPushButton *m_installButton;
    QPushButton *m_uninstallButton;
    QWidget *m_contentWidget;
    QLabel *m_emptyLabel;
    QLabel *m_cmdTitle;
    QStringList m_currentCommands;
    QString m_currentUninstallCommand;
    int m_selectedCount;
    bool m_isDark;
    bool m_updateMode = false;
};

#endif
