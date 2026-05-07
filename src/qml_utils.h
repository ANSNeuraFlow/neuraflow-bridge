#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QUuid>

class QmlUtils : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit QmlUtils(QObject *parent = nullptr);

    Q_INVOKABLE QString generateUuid();
};
