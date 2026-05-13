#pragma once

#include <QByteArray>
#include <cmath>
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
    void setSampleRateFromFrameIntervalMs(int intervalMs);
    /// Sample rate in Hz (e.g. 250 for Cyton; derived from frame interval if using setSampleRateFromFrameIntervalMs).
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
    void parseAndPush(const QByteArray &frame, int maxLen);
    void trimBuffers();
    void recomputeRms();
    void maybeUpdateAutoscale();
    void onRenderTimer();

    static constexpr int kChannelCount = 8;
    static constexpr double kDefaultWindowSec = 5.0;
    /// Fixed ring capacity: max supported rate * max window (see setWindowSeconds clamp) + margin.
    static constexpr int kRingStorageSamples = static_cast<int>(std::ceil(500.0 * 20.0)) + 128;
    static constexpr int kRenderIntervalMs = 50;  // ~20 Hz

    struct ChannelSamplesRing
    {
        QVector<double> storage;
        int head{0};
        int count{0};

        ChannelSamplesRing();
        void push(double value, int maxLen);
        void trimToMax(int maxLen);
        [[nodiscard]] int size() const { return count; }
        [[nodiscard]] double at(int linearIndex) const;
        [[nodiscard]] QVector<double> linearize() const;

    private:
        [[nodiscard]] int capacity() const { return storage.size(); }
    };

    int m_channelCount{kChannelCount};
    double m_sampleRateHz{250.0};  // Cyton default; follows DeviceManager effectiveSampleRateHz when wired in main
    double m_windowSeconds{kDefaultWindowSec};
    /// 0 = Auto (µV symmetric scale derived from buffered signal)
    int m_verticalScaleUv{200};

    QVector<ChannelSamplesRing> m_buffers;

    QVector<double> m_rmsUv;
    QVector<double> m_autoscaleYMin;
    QVector<double> m_autoscaleYMax;

    qint64 m_lastAutoscaleMs{0};
    bool m_dirty{false};
    QTimer m_renderTimer;

    mutable QMutex m_mutex;
};
