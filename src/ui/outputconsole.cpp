#include "ui/outputconsole.h"
#include "core/appsettings.h"
#include "core/theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollBar>

OutputConsole::OutputConsole(QWidget *parent)
    : QWidget(parent)
{
    setThemeColors(true);
    setupUI();
}

void OutputConsole::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_headerWidget = new QWidget(this);
    m_headerWidget->setObjectName("consoleHeader");
    auto *headerLayout = new QHBoxLayout(m_headerWidget);
    headerLayout->setContentsMargins(12, 4, 8, 4);

    auto *titleLabel = new QLabel("Terminal", this);
    titleLabel->setObjectName("consoleTitle");

    m_closeButton = new QPushButton("Fechar", this);
    m_closeButton->setObjectName("consoleCloseBtn");
    m_closeButton->setFixedSize(70, 24);

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(m_closeButton);

    m_output = new QTextEdit(this);
    m_output->setReadOnly(true);
    m_output->setObjectName("consoleOutput");
    m_output->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_output->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_output->setMinimumHeight(120);
    m_output->setMaximumHeight(AppSettings::consoleMaxHeight());

    m_inputLine = new QLineEdit(this);
    m_inputLine->setObjectName("consoleInput");
    m_inputLine->setPlaceholderText("Entrada para o processo (Enter para enviar)...");
    m_inputLine->setEnabled(false);

    mainLayout->addWidget(m_headerWidget);
    mainLayout->addWidget(m_output);
    mainLayout->addWidget(m_inputLine);

    connect(m_closeButton, &QPushButton::clicked, this, &OutputConsole::onCloseClicked);
    connect(m_inputLine, &QLineEdit::returnPressed, this, &OutputConsole::onInputSubmitted);

    hide();
}

void OutputConsole::setThemeColors(bool dark)
{
    auto c = dark ? darkTheme() : lightTheme();
    m_colorInfo = c.accent;
    m_colorError = c.red;
    m_colorSuccess = c.green;
    m_colorDefault = c.fg1;

    setStyleSheet(QString(R"(
        #consoleHeader {
            background-color: %1;
            border-top: 1px solid %2;
            border-bottom: 1px solid %2;
        }
        #consoleTitle {
            color: %3;
            font-size: 11px;
            font-weight: bold;
        }
        #consoleCloseBtn {
            background-color: %4;
            color: %5;
            border: none;
            border-radius: 4px;
            font-size: 11px;
            padding: 2px 8px;
        }
        #consoleCloseBtn:hover {
            background-color: %6;
        }
        #consoleOutput {
            background-color: %1;
            color: %5;
            font-family: "Noto Sans Mono", "monospace", "Courier New";
            font-size: 12px;
            border: none;
            padding: 8px;
        }
        #consoleInput {
            background-color: %7;
            color: %5;
            font-family: "Noto Sans Mono", "monospace", "Courier New";
            font-size: 12px;
            border: none;
            border-top: 1px solid %2;
            padding: 6px 8px;
        }
        #consoleInput:disabled {
            color: %8;
        }
    )").arg(c.bg3, c.border, c.fg2, c.hoverBg, c.fg1, c.fg3, c.bg2, c.fg3));
}

void OutputConsole::attachProcess(QProcess *process)
{
    if (m_activeProcess)
        disconnect(m_activeProcess.data(), nullptr, this, nullptr);

    m_activeProcess = process;
    m_readBuffer.clear();
    m_inputLine->clear();
    m_inputLine->setEnabled(true);
    m_inputLine->setFocus();
    setPasswordMode(false);

    connect(process, &QProcess::readyReadStandardOutput,
            this, &OutputConsole::onReadyReadStdout);
    connect(process, &QProcess::readyReadStandardError,
            this, &OutputConsole::onReadyReadStderr);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &OutputConsole::onProcessFinished);
}

void OutputConsole::appendOutput(const QString &text, const QColor &color)
{
    QString html = QString("<span style='color: %1'>%2</span>")
                       .arg(color.name(), text.toHtmlEscaped());
    m_output->append(html);
    auto *sb = m_output->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void OutputConsole::appendInfo(const QString &text)
{
    appendOutput(QString("[INFO] %1").arg(text), QColor(m_colorInfo));
}

void OutputConsole::appendError(const QString &text)
{
    appendOutput(QString("[ERRO] %1").arg(text), QColor(m_colorError));
}

void OutputConsole::appendSuccess(const QString &text)
{
    appendOutput(QString("[OK] %1").arg(text), QColor(m_colorSuccess));
}

void OutputConsole::clear()
{
    m_output->clear();
    m_inputLine->clear();
}

void OutputConsole::onReadyReadStdout()
{
    auto *proc = qobject_cast<QProcess *>(sender());
    if (!proc) return;

    m_readBuffer += QString::fromUtf8(proc->readAllStandardOutput());

    int idx;
    while ((idx = m_readBuffer.indexOf('\n')) != -1) {
        QString line = m_readBuffer.left(idx);
        m_readBuffer = m_readBuffer.mid(idx + 1);
        appendLine(line);
    }

    // Um prompt de senha nao termina em '\n': sem isso ele ficava preso no
    // buffer e o usuario nao via o pedido de senha.
    if (!m_readBuffer.isEmpty() && looksLikePasswordPrompt(m_readBuffer)) {
        appendLine(m_readBuffer);
        m_readBuffer.clear();
        setPasswordMode(true);
    }
}

bool OutputConsole::looksLikePasswordPrompt(const QString &text)
{
    const QString lower = text.trimmed().toLower();
    if (lower.isEmpty())
        return false;
    return lower.contains("[sudo]")
        || lower.contains("password")
        || lower.contains("senha")
        || lower.contains("passphrase");
}

void OutputConsole::setPasswordMode(bool enabled)
{
    if (m_passwordMode == enabled)
        return;

    m_passwordMode = enabled;
    m_inputLine->setEchoMode(enabled ? QLineEdit::Password : QLineEdit::Normal);
    m_inputLine->setPlaceholderText(enabled
        ? QStringLiteral("Senha (não será exibida)...")
        : QStringLiteral("Entrada para o processo (Enter para enviar)..."));
}

void OutputConsole::appendLine(const QString &text)
{
    if (text.isEmpty()) return;

    static const QStringList errorPatterns = {
        QStringLiteral("erro:"), QStringLiteral("error:"),
        QStringLiteral("==> ERRO:"), QStringLiteral("fatal:"),
        QStringLiteral("Failed"), QStringLiteral("Aborting"),
        QStringLiteral("Abortando"), QStringLiteral("ERRO:")
    };

    QColor color(m_colorDefault);
    for (const auto &p : errorPatterns) {
        if (text.contains(p, Qt::CaseInsensitive)) {
            color = QColor(m_colorError);
            break;
        }
    }

    appendOutput(text + QStringLiteral("\n"), color);
}

void OutputConsole::onReadyReadStderr()
{
    auto *proc = qobject_cast<QProcess *>(sender());
    if (!proc) return;
    QString data = QString::fromUtf8(proc->readAllStandardError());
    appendOutput(data, QColor(m_colorError));
}

void OutputConsole::onInputSubmitted()
{
    if (!m_activeProcess || m_activeProcess->state() != QProcess::Running)
        return;

    QString input = m_inputLine->text();
    if (input.isEmpty()) return;

    m_inputLine->clear();
    // Uma senha nunca vai para o transcript do console.
    appendOutput(m_passwordMode
                     ? QStringLiteral("> ********\n")
                     : QString("> %1\n").arg(input),
                 QColor(m_colorInfo));
    m_activeProcess->write((input + "\n").toUtf8());
    m_activeProcess->waitForBytesWritten(1000);
    setPasswordMode(false);
}

void OutputConsole::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    if (!m_readBuffer.isEmpty()) {
        appendLine(m_readBuffer);
        m_readBuffer.clear();
    }

    m_inputLine->setEnabled(false);
    m_activeProcess = nullptr;
    setPasswordMode(false);

    if (status == QProcess::NormalExit && exitCode == 0) {
        appendSuccess("Processo concluído com sucesso.");
    } else {
        appendError(QString("Processo falhou (código: %1).").arg(exitCode));
    }
}

void OutputConsole::onCloseClicked()
{
    hide();
}
