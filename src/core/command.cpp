#include "core/command.h"

namespace {

// Aspas simples so quando o token tem algo que um shell interpretaria.
QString quoteIfNeeded(const QString &token)
{
    if (token.isEmpty())
        return QStringLiteral("''");

    static const QString safe = QStringLiteral(
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
        "_@%+=:,./-");

    bool needsQuotes = false;
    for (const QChar &c : token) {
        if (!safe.contains(c)) {
            needsQuotes = true;
            break;
        }
    }
    if (!needsQuotes)
        return token;

    QString escaped = token;
    escaped.replace(QLatin1String("'"), QLatin1String("'\\''"));
    return QLatin1Char('\'') + escaped + QLatin1Char('\'');
}

} // namespace

Command::Command(QString program, QStringList arguments,
                 QMap<QString, QString> environment)
    : m_program(std::move(program))
    , m_arguments(std::move(arguments))
    , m_environment(std::move(environment))
{
}

QString Command::toString() const
{
    if (m_program.isEmpty())
        return {};

    QStringList parts;
    parts.reserve(m_arguments.size() + m_environment.size() + 1);

    for (auto it = m_environment.cbegin(); it != m_environment.cend(); ++it)
        parts << it.key() + QLatin1Char('=') + quoteIfNeeded(it.value());

    parts << quoteIfNeeded(m_program);
    for (const QString &arg : m_arguments)
        parts << quoteIfNeeded(arg);

    return parts.join(QLatin1Char(' '));
}

bool Command::operator==(const Command &other) const
{
    return m_program == other.m_program
        && m_arguments == other.m_arguments
        && m_environment == other.m_environment;
}
