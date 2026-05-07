#include "qml_utils.h"

QmlUtils::QmlUtils(QObject *parent) {}

QString QmlUtils::generateUuid()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}
