#pragma once

#include <QColor>
#include <QPointer>
#include <QQuickPaintedItem>
#include <QVector>
#include <QtQml/qqmlregistration.h>
#include <memory>

class QCustomPlot;

/// Renders an off-screen qcustomplot into QML (OpenBCI Time Series style curve).
class QCustomPlotQuickItem : public QQuickPaintedItem
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(QColor lineColor READ lineColor WRITE setLineColor NOTIFY lineColorChanged)
  Q_PROPERTY(bool bottomAxisVisible READ bottomAxisVisible WRITE setBottomAxisVisible NOTIFY
                 bottomAxisVisibleChanged)

public:
  explicit QCustomPlotQuickItem(QQuickItem *parent = nullptr);
  ~QCustomPlotQuickItem() override;

  QColor lineColor() const { return m_lineColor; }
  void setLineColor(const QColor &c);

  bool bottomAxisVisible() const { return m_bottomAxisVisible; }
  void setBottomAxisVisible(bool v);

  Q_INVOKABLE void setSamples(const QVariantList &xs, const QVariantList &ys);
  void setData(const QVector<double> &xs, const QVector<double> &ys);
  void setData(QVector<double> &&xs, QVector<double> &&ys);
  Q_INVOKABLE void setYRange(double yMin, double yMax);
  Q_INVOKABLE void setXRange(double xMin, double xMax);

  void beginBatchUpdate();
  void endBatchUpdate();

  void paint(QPainter *painter) override;

signals:
  void lineColorChanged();
  void bottomAxisVisibleChanged();

protected:
  void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
  void ensurePlot();
  void applyStyle();
  void replotToPainter(QPainter *painter);
  void scheduleUpdate();

  std::unique_ptr<QCustomPlot> m_plot;
  QColor m_lineColor{QStringLiteral("#3b82f6")};
  bool m_bottomAxisVisible{false};
  bool m_batchUpdate{false};
  QVector<double> m_cachedX;
  QVector<double> m_cachedY;
};
