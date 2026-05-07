#include "utils.h"

QVariantMap readSettingsGroup(QSettings &settings, const QString &group) {
    QVariantMap map;

    settings.beginGroup(group);

    const QStringList keys = settings.childKeys();

    for (const QString &key : keys) {
        map.insert(key, settings.value(key));
    }

    settings.endGroup();

    return map;
}
