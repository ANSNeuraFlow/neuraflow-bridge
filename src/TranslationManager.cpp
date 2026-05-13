#include "TranslationManager.h"

#include <QDebug>

TranslationManager::TranslationManager(QGuiApplication *app, QQmlApplicationEngine *engine)
    : m_app(app), m_engine(engine) {}

bool TranslationManager::setLanguage(const QString &locale)
{
    if (!m_translator.isEmpty())
    {
        m_app->removeTranslator(&m_translator);
    }

    QString languageCode = locale;
    const int separatorIndex = languageCode.indexOf('_');
    if (separatorIndex > 0)
    {
        languageCode = languageCode.left(separatorIndex);
    }

    QString filename = QString(":/qt/qml/NeuraFlowBridge/i18n/qml_%1.qm").arg(languageCode);

    if (m_translator.load(filename))
    {
        m_app->installTranslator(&m_translator);
        m_engine->retranslate();

        qDebug() << "Language switched to:" << locale << "(resource:" << filename << ")";
        return true;
    }

    qWarning() << "Failed to load translation file:" << filename;
    return false;
}
