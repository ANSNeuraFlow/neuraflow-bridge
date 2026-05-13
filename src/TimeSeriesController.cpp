#include "TimeSeriesController.h"

#include "DataProcessor.h"
#include "QCustomPlotQuickItem.h"

#include <array>

namespace
{
// Match OpenBCI_GUI TimeSeriesXLim labels order
constexpr std::array<double, 5> kWindowSeconds{{1., 3., 5., 10., 20.}};
// Match TimeSeriesYLim: Auto then fixed µV
constexpr std::array<int, 7> kVertScaleUv{{0, 50, 100, 200, 400, 1000, 10000}};

const std::array<QColor, 8> kPalette{{
    QColor(QStringLiteral("#3b82f6")),
    QColor(QStringLiteral("#10b981")),
    QColor(QStringLiteral("#f59e0b")),
    QColor(QStringLiteral("#8b5cf6")),
    QColor(QStringLiteral("#ec4899")),
    QColor(QStringLiteral("#14b8a6")),
    QColor(QStringLiteral("#f97316")),
    QColor(QStringLiteral("#06b6d4")),
}};
}

TimeSeriesController::TimeSeriesController(DataProcessor *processor, QObject *parent)
    : QObject(parent), m_processor(processor),
      m_visible(static_cast<QVector<bool>::size_type>(numChannels()), true)
{
  if (!m_processor)
    return;
  syncProcessorWindow();
  syncProcessorVertScale();
}

QVariantList TimeSeriesController::activeChannels() const
{
  QVariantList out;
  for (int i = 0; i < numChannels(); ++i)
  {
    if (i < m_visible.size() && m_visible[static_cast<QVector<bool>::size_type>(i)])
      out.append(i);
  }
  return out;
}

void TimeSeriesController::setHorizontalWindowIndex(int idx)
{
  if (idx < 0 || idx >= static_cast<int>(kWindowSeconds.size()))
    return;
  if (m_horizontalWindowIndex == idx)
    return;
  m_horizontalWindowIndex = idx;
  syncProcessorWindow();
  emit horizontalWindowIndexChanged();
}

void TimeSeriesController::setVertScaleIndex(int idx)
{
  if (idx < 0 || idx >= static_cast<int>(kVertScaleUv.size()))
    return;
  if (m_vertScaleIndex == idx)
    return;
  m_vertScaleIndex = idx;
  syncProcessorVertScale();
  emit vertScaleIndexChanged();
}

void TimeSeriesController::syncProcessorWindow()
{
  if (!m_processor)
    return;
  m_processor->setWindowSeconds(kWindowSeconds[static_cast<std::size_t>(m_horizontalWindowIndex)]);
}

void TimeSeriesController::syncProcessorVertScale()
{
  if (!m_processor)
    return;
  m_processor->setVerticalScaleUv(kVertScaleUv[static_cast<std::size_t>(m_vertScaleIndex)]);
}

QColor TimeSeriesController::channelColor(int channelIndex) const
{
  if (channelIndex < 0 || channelIndex >= numChannels())
    return QColor(Qt::white);
  return kPalette[static_cast<std::size_t>(channelIndex % kPalette.size())];
}

bool TimeSeriesController::channelVisible(int channelIndex) const
{
  if (channelIndex < 0 || channelIndex >= m_visible.size())
    return false;
  return m_visible[static_cast<QVector<bool>::size_type>(channelIndex)];
}

void TimeSeriesController::toggleChannelVisibility(int channelIndex)
{
  if (channelIndex < 0 || channelIndex >= m_visible.size())
    return;
  auto &v = m_visible[static_cast<QVector<bool>::size_type>(channelIndex)];
  v = !v;
  emit activeChannelsChanged();
}

double TimeSeriesController::yMinForChannel(int channelIndex) const
{
  if (!m_processor || channelIndex < 0 || channelIndex >= numChannels())
    return -200.0;

  if (m_processor->autoscaleY())
  {
    const QVariantList mins = m_processor->autoscaleYMin();
    if (channelIndex < mins.size())
      return mins.at(channelIndex).toDouble();
    return -200.0;
  }

  const int sym = m_processor->verticalScaleUv();
  return static_cast<double>(-sym);
}

double TimeSeriesController::yMaxForChannel(int channelIndex) const
{
  if (!m_processor || channelIndex < 0 || channelIndex >= numChannels())
    return 200.0;

  if (m_processor->autoscaleY())
  {
    const QVariantList maxs = m_processor->autoscaleYMax();
    if (channelIndex < maxs.size())
      return maxs.at(channelIndex).toDouble();
    return 200.0;
  }

  const int sym = m_processor->verticalScaleUv();
  return static_cast<double>(sym);
}

void TimeSeriesController::updatePlot(QCustomPlotQuickItem *plot, int channelIndex) const
{
  if (!plot || !m_processor)
    return;

  if (channelIndex < 0 || channelIndex >= numChannels() ||
      !channelVisible(channelIndex))
  {
    plot->setData({}, {});
    return;
  }

  const QVector<double> xs = m_processor->timeAxisSecondsVec(channelIndex);
  const QVector<double> ys = m_processor->channelSamplesVec(channelIndex);
  plot->setData(xs, ys);
  plot->setXRange(-m_processor->windowSeconds(), 0);
  plot->setYRange(yMinForChannel(channelIndex), yMaxForChannel(channelIndex));
}
