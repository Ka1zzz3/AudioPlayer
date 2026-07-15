#include "Common/ViewCommand.h"

#include <utility>

namespace AudioPlayer::Common {

ViewCommand::ViewCommand(QObject *parent)
    : QObject(parent)
{
}

ViewCommand::ViewCommand(QString name, ExecuteHandler executeHandler, QObject *parent)
    : QObject(parent)
    , m_name(std::move(name))
    , m_executeHandler(std::move(executeHandler))
{
}

bool ViewCommand::isEnabled() const noexcept
{
    return m_enabled;
}

void ViewCommand::setEnabled(bool enabled)
{
    if (m_enabled == enabled) {
        return;
    }

    m_enabled = enabled;
    emit enabledChanged();
}

const QString &ViewCommand::name() const noexcept
{
    return m_name;
}

bool ViewCommand::execute()
{
    if (!m_enabled || !m_executeHandler) {
        emit executed(false);
        return false;
    }

    const bool succeeded = m_executeHandler();
    emit executed(succeeded);
    return succeeded;
}

} // namespace AudioPlayer::Common
