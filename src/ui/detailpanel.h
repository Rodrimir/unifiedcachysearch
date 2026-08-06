#ifndef DETAILPANEL_H
#define DETAILPANEL_H

#include <QLabel>
#include <QPushButton>
#include <QWidget>
#include "core/command.h"
#include "core/searchitem.h"

class DetailPanel : public QWidget
{
    Q_OBJECT

public:
    explicit DetailPanel(QWidget *parent = nullptr);

    // O comando de remocao vem pronto de quem sabe construi-lo (o engine);
    // a UI nao monta linha de comando.
    void showPackage(const SearchItem &item, const Command &uninstallCommand);
    void setSelectedCount(int count);
    void setUpdateMode(bool updateMode);
    Command currentCommand() const { return m_installCommand; }
    void clear();
    void setThemeColors(bool dark);

signals:
    void installRequested();
    void uninstallRequested(const Command &command);

private:
    void setupUI();
    void updateInstallButtonText();

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

    Command m_installCommand;
    Command m_uninstallCommand;
    bool m_hasPackage = false;
    bool m_currentInstalled = false;
    int m_selectedCount = 0;
    bool m_isDark = true;
    bool m_updateMode = false;
};

#endif
