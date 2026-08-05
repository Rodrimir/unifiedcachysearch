#ifndef OUTPUTCONSOLE_H
#define OUTPUTCONSOLE_H

#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QProcess>
#include <QPointer>

class OutputConsole : public QWidget
{
    Q_OBJECT

public:
    explicit OutputConsole(QWidget *parent = nullptr);

    void attachProcess(QProcess *process);
    void appendOutput(const QString &text, const QColor &color = QColor("#cdd6f4"));
    void appendInfo(const QString &text);
    void appendError(const QString &text);
    void appendSuccess(const QString &text);
    void setThemeColors(bool dark);
    void clear();

private slots:
    void onReadyReadStdout();
    void onReadyReadStderr();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onInputSubmitted();
    void onCloseClicked();

private:
    void setupUI();
    void appendLine(const QString &text);

    QTextEdit *m_output;
    QLineEdit *m_inputLine;
    QPushButton *m_closeButton;
    QWidget *m_headerWidget;
    QPointer<QProcess> m_activeProcess;
    QString m_readBuffer;
    QString m_colorInfo, m_colorError, m_colorSuccess, m_colorDefault;
};

#endif
