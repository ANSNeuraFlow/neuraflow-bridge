#pragma once

#include <QByteArray>
#include <QElapsedTimer>
#include <QMutex>
#include <QObject>
#include <QTimer>
#include <QVector>
#include <QVariant>

class DataProcessor final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int channelCount READ channelCount CONSTANT)
    Q_PROPERTY(double sampleRateHz READ sampleRateHz NOTIFY sampleRateHzChanged)
    Q_PROPERTY(double windowSeconds READ windowSeconds WRITE setWindowSeconds NOTIFY windowSecondsChanged)
    Q_PROPERTY(int verticalScaleUv READ verticalScaleUv WRITE setVerticalScaleUv NOTIFY verticalScaleUvChanged)
    Q_PROPERTY(bool autoscaleY READ autoscaleY NOTIFY verticalScaleUvChanged)
    Q_PROPERTY(QVariantList rmsMicrovolts READ rmsMicrovolts NOTIFY rmsMicrovoltsChanged)
    Q_PROPERTY(QVariantList autoscaleYMin READ autoscaleYMin NOTIFY autoscaleBoundsChanged)
    Q_PROPERTY(QVariantList autoscaleYMax READ autoscaleYMax NOTIFY autoscaleBoundsChanged)

public:
    explicit DataProcessor(QObject *parent = nullptr);

    int channelCount() const { return m_channelCount; }
    double sampleRateHz() const { return m_sampleRateHz; }
    double windowSeconds() const { return m_windowSeconds; }
    void setWindowSeconds(double seconds);
    int verticalScaleUv() const { return m_verticalScaleUv; }
    void setVerticalScaleUv(int microvolts);
    bool autoscaleY() const { return m_verticalScaleUv == 0; }

    QVariantList rmsMicrovolts() const;
    QVariantList autoscaleYMin() const { return QVariantList{m_autoscaleYMin.constBegin(), m_autoscaleYMin.constEnd()}; }
    QVariantList autoscaleYMax() const { return QVariantList{m_autoscaleYMax.constBegin(), m_autoscaleYMax.constEnd()}; }

public slots:
    void ingestFrame(const QByteArray &frame);
    /// Match DeviceManager synthetic frame cadence unless set explicitly.
    void setSampleRateFromFrameIntervalMs(int intervalMs);
    /// Sample rate in Hz (e.g. 250 for Cyton, 100 for 10 ms synthetic timer).
    void setSampleRateHz(double hz);

    Q_INVOKABLE QVariantList timeAxisSeconds(int channelIndex) const;
    Q_INVOKABLE QVariantList channelSamples(int channelIndex) const;

    QVector<double> timeAxisSecondsVec(int channelIndex) const;
    QVector<double> channelSamplesVec(int channelIndex) const;

signals:
    void sampleRateHzChanged();
    void windowSecondsChanged();
    void verticalScaleUvChanged();
    void rmsMicrovoltsChanged();
    void autoscaleBoundsChanged();
    void dataUpdated();
    void renderTick();

private:
    void parseAndPush(const QByteArray &frame);
    void trimBuffers();
    void recomputeRms();
    void maybeUpdateAutoscale();
    void onRenderTimer();

    static constexpr int kChannelCount = 8;
    static constexpr double kDefaultWindowSec = 5.0;
    static constexpr int kRenderIntervalMs = 33;  // ~30 Hz

    int m_channelCount{kChannelCount};
    double m_sampleRateHz{250.0};  // Cyton default; synthetic/debug uses DeviceManager rate
    double m_windowSeconds{kDefaultWindowSec};
    /// 0 = Auto (µV symmetric scale derived from buffered signal)
    int m_verticalScaleUv{200};

    QVector<QVector<double>> m_buffers;  // per-channel time series newest at end

    QVector<double> m_rmsUv;
    QVector<double> m_autoscaleYMin;
    QVector<double> m_autoscaleYMax;

    qint64 m_lastAutoscaleMs{0};
    bool m_dirty{false};
    QTimer m_renderTimer;

    mutable QMutex m_mutex;
};
