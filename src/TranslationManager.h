#pragma once

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTranslator>

class TranslationManager : public QObject {
    Q_OBJECT

public:
    explicit TranslationManager(QGuiApplication* app, QQmlApplicationEngine* engine);

    Q_INVOKABLE bool setLanguage(const QString& locale);

private:
    QGuiApplication* m_app;
    QQmlApplicationEngine* m_engine;
    QTranslator m_translator;
};
