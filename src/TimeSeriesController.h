#pragma once

#include <QColor>
#include <QObject>
#include <QVariantList>

class DataProcessor;
class QCustomPlotQuickItem;

class TimeSeriesController final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int numChannels READ numChannels CONSTANT)
  /// Visible channel indices in order (subset of all channels).
  Q_PROPERTY(QVariantList activeChannels READ activeChannels NOTIFY activeChannelsChanged)
  Q_PROPERTY(int horizontalWindowIndex READ horizontalWindowIndex WRITE setHorizontalWindowIndex NOTIFY
                  horizontalWindowIndexChanged)
  Q_PROPERTY(int vertScaleIndex READ vertScaleIndex WRITE setVertScaleIndex NOTIFY vertScaleIndexChanged)

public:
  explicit TimeSeriesController(DataProcessor *processor, QObject *parent = nullptr);

  int numChannels() const { return 8; }

  QVariantList activeChannels() const;

  int horizontalWindowIndex() const { return m_horizontalWindowIndex; }
  void setHorizontalWindowIndex(int idx);

  int vertScaleIndex() const { return m_vertScaleIndex; }
  void setVertScaleIndex(int idx);

  Q_INVOKABLE QColor channelColor(int channelIndex) const;
  Q_INVOKABLE bool channelVisible(int channelIndex) const;
  Q_INVOKABLE void toggleChannelVisibility(int channelIndex);

  /// For plot Y range — uses symmetric limits or autoscale slices from processor.
  Q_INVOKABLE double yMinForChannel(int channelIndex) const;
  Q_INVOKABLE double yMaxForChannel(int channelIndex) const;

  Q_INVOKABLE void updatePlot(QCustomPlotQuickItem *plot, int channelIndex) const;

signals:
  void activeChannelsChanged();
  void horizontalWindowIndexChanged();
  void vertScaleIndexChanged();

private:
  void syncProcessorWindow();
  void syncProcessorVertScale();

  DataProcessor *m_processor{};
  QVector<bool> m_visible;
  int m_horizontalWindowIndex{2};  /// default "5 sec" (index into {1,3,5,10,20})
  int m_vertScaleIndex{3};         /// default 200 µV (index into enums matching DataProcessor presets)
};
