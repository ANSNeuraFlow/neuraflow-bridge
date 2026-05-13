#include "DataProcessor.h"

#include <algorithm>
#include <QDataStream>
#include <QDateTime>
#include <cmath>

namespace
{
constexpr qint64 kAutoscaleThrottleMs = 1000;
}

DataProcessor::DataProcessor(QObject *parent)
    : QObject(parent),
      m_buffers(static_cast<QVector<QVector<double>>::size_type>(kChannelCount)),
      m_rmsUv(kChannelCount, 0.0),
      m_autoscaleYMin(kChannelCount, -1.0),
      m_autoscaleYMax(kChannelCount, 1.0)
{
  for (int i = 0; i < kChannelCount; ++i)
  {
    m_buffers[i].reserve(static_cast<QVector<QVector<double>>::size_type>(
        std::ceil(m_sampleRateHz * m_windowSeconds) + 32));
    m_autoscaleYMin[i] = -200.0;
    m_autoscaleYMax[i] = 200.0;
  }

  m_renderTimer.setInterval(kRenderIntervalMs);
  m_renderTimer.setTimerType(Qt::PreciseTimer);
  connect(&m_renderTimer, &QTimer::timeout, this, &DataProcessor::onRenderTimer);
}

void DataProcessor::setSampleRateFromFrameIntervalMs(int intervalMs)
{
  if (intervalMs <= 0)
    return;
  const double hz = 1000.0 / static_cast<double>(intervalMs);
  setSampleRateHz(hz);
}

void DataProcessor::setSampleRateHz(double hz)
{
  if (!(hz > 0.0))
    return;

  bool changed = false;
  {
    const QMutexLocker lock(&m_mutex);
    if (std::fabs(m_sampleRateHz - hz) < 1e-6)
      return;
    m_sampleRateHz = hz;
    changed = true;
  }
  if (changed)
    emit sampleRateHzChanged();
}

void DataProcessor::setWindowSeconds(double seconds)
{
  constexpr double minS = 1.0;
  constexpr double maxS = 20.0;
  const double clamped = std::clamp(seconds, minS, maxS);

  bool changed = false;
  {
    const QMutexLocker lock(&m_mutex);
    if (std::fabs(m_windowSeconds - clamped) < 1e-9)
      return;
    m_windowSeconds = clamped;
    trimBuffers();
    changed = true;
  }
  if (!changed)
    return;

  emit windowSecondsChanged();
  emit dataUpdated();
  maybeUpdateAutoscale();
}

void DataProcessor::setVerticalScaleUv(int microvolts)
{
  constexpr int presets[] = {0, 50, 100, 200, 400, 1000, 10000};
  const bool valid =
      microvolts == 0 ||
      std::any_of(std::begin(presets), std::end(presets),
                  [microvolts](int v) { return v == microvolts; });
  if (!valid)
    return;

  bool changed = false;
  {
    const QMutexLocker lock(&m_mutex);
    if (m_verticalScaleUv == microvolts)
      return;
    m_verticalScaleUv = microvolts;
    changed = true;
  }
  if (!changed)
    return;

  emit verticalScaleUvChanged();
  emit autoscaleBoundsChanged();
}

void DataProcessor::parseAndPush(const QByteArray &frame)
{
  // Synthetic / bridge frame: qint64 ts LE, quint32 seq LE, kChannelCount floats LE
  constexpr int payload =
      static_cast<int>(sizeof(qint64) + sizeof(quint32) + sizeof(float) * kChannelCount);
  if (frame.size() < payload)
    return;

  QDataStream ds(frame);
  ds.setByteOrder(QDataStream::LittleEndian);

  qint64 tsMs = 0;
  quint32 seq = 0;
  ds >> tsMs >> seq;
  Q_UNUSED(seq);
  Q_UNUSED(tsMs);

  for (int i = 0; i < kChannelCount; ++i)
  {
    float v = 0.f;
    ds >> v;
    m_buffers[i].push_back(static_cast<double>(v));
  }
}

void DataProcessor::trimBuffers()
{
  const int maxSamples =
      static_cast<int>(std::ceil(m_sampleRateHz * m_windowSeconds)) + 4;
  for (int ch = 0; ch < kChannelCount; ++ch)
  {
    while (m_buffers[ch].size() > maxSamples)
      m_buffers[ch].removeFirst();
  }
}

void DataProcessor::recomputeRms()
{
  const QMutexLocker lock(&m_mutex);
  for (int ch = 0; ch < kChannelCount; ++ch)
  {
    const QVector<double> &buf = m_buffers[ch];
    if (buf.isEmpty())
    {
      m_rmsUv[ch] = 0.0;
      continue;
    }
    double sumSq = 0.0;
    for (double x : buf)
      sumSq += x * x;
    m_rmsUv[ch] = std::sqrt(sumSq / static_cast<double>(buf.size()));
  }
}

void DataProcessor::maybeUpdateAutoscale()
{
  int scaleUv = 0;
  qint64 lastMs = 0;
  {
    const QMutexLocker lock(&m_mutex);
    scaleUv = m_verticalScaleUv;
    lastMs = m_lastAutoscaleMs;
  }

  if (scaleUv != 0)
    return;

  const qint64 now = QDateTime::currentMSecsSinceEpoch();
  if (now - lastMs < kAutoscaleThrottleMs)
    return;

  {
    const QMutexLocker lock(&m_mutex);
    if (m_verticalScaleUv != 0)
      return;
    m_lastAutoscaleMs = now;

    for (int ch = 0; ch < kChannelCount; ++ch)
    {
      const QVector<double> &buf = m_buffers[ch];
      if (buf.isEmpty())
      {
        m_autoscaleYMin[ch] = -200.0;
        m_autoscaleYMax[ch] = 200.0;
        continue;
      }

      const auto range = std::minmax_element(buf.constBegin(), buf.constEnd());
      double lo = std::floor(*range.first);
      double hi = std::ceil(*range.second);
      const double pad = std::max(1.0, (hi - lo) * 0.05);
      lo -= pad;
      hi += pad;
      const double extent = std::max(std::abs(lo), std::abs(hi));
      m_autoscaleYMin[ch] = -extent;
      m_autoscaleYMax[ch] = extent;
    }
  }
  emit autoscaleBoundsChanged();
}

void DataProcessor::ingestFrame(const QByteArray &frame)
{
  {
    const QMutexLocker lock(&m_mutex);
    parseAndPush(frame);
    trimBuffers();
    m_dirty = true;
  }

  if (!m_renderTimer.isActive())
  {
    m_renderTimer.start();
  }
}

void DataProcessor::onRenderTimer()
{
  bool wasDirty = false;
  {
    const QMutexLocker lock(&m_mutex);
    wasDirty = m_dirty;
    m_dirty = false;
  }

  if (!wasDirty)
  {
    m_renderTimer.stop();
    return;
  }

  recomputeRms();
  maybeUpdateAutoscale();
  emit renderTick();
  emit rmsMicrovoltsChanged();
}

QVariantList DataProcessor::rmsMicrovolts() const
{
  const QMutexLocker lock(&m_mutex);
  QVariantList list;
  list.reserve(kChannelCount);
  for (int i = 0; i < kChannelCount; ++i)
    list.append(m_rmsUv[i]);
  return list;
}

QVector<double> DataProcessor::timeAxisSecondsVec(int channelIndex) const
{
  const QMutexLocker lock(&m_mutex);
  QVector<double> xs;
  if (channelIndex < 0 || channelIndex >= kChannelCount)
    return xs;

  const QVector<double> &buf = m_buffers[channelIndex];
  const int n = buf.size();
  if (n == 0)
    return xs;

  xs.resize(n);
  const double dt = 1.0 / m_sampleRateHz;
  const double window = m_windowSeconds;
  const double tail = static_cast<double>(n - 1) * dt;
  const double t0 = -std::min(window, tail);

  for (int i = 0; i < n; ++i)
    xs[i] = static_cast<double>(i) * dt + t0;

  return xs;
}

QVector<double> DataProcessor::channelSamplesVec(int channelIndex) const
{
  const QMutexLocker lock(&m_mutex);
  if (channelIndex < 0 || channelIndex >= kChannelCount)
    return {};
  return m_buffers[channelIndex];
}

QVariantList DataProcessor::timeAxisSeconds(int channelIndex) const
{
  const QVector<double> vec = timeAxisSecondsVec(channelIndex);
  QVariantList xs;
  xs.reserve(vec.size());
  for (double v : vec)
    xs.append(v);
  return xs;
}

QVariantList DataProcessor::channelSamples(int channelIndex) const
{
  const QVector<double> vec = channelSamplesVec(channelIndex);
  QVariantList ys;
  ys.reserve(vec.size());
  for (double v : vec)
    ys.append(v);
  return ys;
}
