#pragma once

#include <QObject>
#include <QString>
#include <functional>
#if __has_include(<QtQmlIntegration/qqmlintegration.h>)
#include <QtQmlIntegration/qqmlintegration.h>
#else
#define QML_ANONYMOUS
#endif

namespace AudioPlayer::Common {

class ViewCommand : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QString name READ name CONSTANT)

public:
    using ExecuteHandler = std::function<bool()>;

    explicit ViewCommand(QObject *parent = nullptr);
    ViewCommand(QString name, ExecuteHandler executeHandler, QObject *parent = nullptr);

    [[nodiscard]] bool isEnabled() const noexcept;
    void setEnabled(bool enabled);
    [[nodiscard]] const QString &name() const noexcept;

public slots:
    bool execute();

signals:
    void enabledChanged();
    void executed(bool succeeded);

private:
    QString m_name;
    ExecuteHandler m_executeHandler;
    bool m_enabled = true;
};

} // namespace AudioPlayer::Common
