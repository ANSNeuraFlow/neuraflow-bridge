#include "QmlUtils.h"

QmlUtils::QmlUtils(QObject *parent) {}

QString QmlUtils::generateUuid()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}
