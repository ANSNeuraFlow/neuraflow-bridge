#pragma once

#include <QSettings>

template<typename T>
T getSetting(QSettings &settings, const QString &key, bool required, T defaultValue = T()) {
    if (settings.contains(key)) {
        return settings.value(key).value<T>();
    }

    if (!required) {
        qWarning().noquote() << "Optional setting '" << key << "' not defined. Using default value: '" << QVariant::fromValue(defaultValue).toString() << "'";
        return defaultValue;
    }

    qCritical().noquote() << "Required setting '" << key << "' is not defined in config.ini. Aborting.";
    exit(-1);
}

QVariantMap readSettingsGroup(QSettings &settings, const QString &group);
