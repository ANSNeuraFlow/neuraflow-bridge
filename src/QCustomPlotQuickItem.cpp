#include "QCustomPlotQuickItem.h"

#include "qcustomplot.h"

#include <QBrush>
#include <QFont>
#include <QMargins>
#include <QPainter>
#include <QtMath>

QCustomPlotQuickItem::QCustomPlotQuickItem(QQuickItem *parent) : QQuickPaintedItem(parent)
{
  setAntialiasing(true);
  setMipmap(false);
}

QCustomPlotQuickItem::~QCustomPlotQuickItem() = default;

void QCustomPlotQuickItem::ensurePlot()
{
  if (m_plot)
    return;
  m_plot = std::make_unique<QCustomPlot>();
  m_plot->setAttribute(Qt::WA_DontShowOnScreen, true);
  m_plot->setParent(nullptr);
  m_plot->resize(1, 1);
  m_plot->addGraph();

  applyStyle();
}

void QCustomPlotQuickItem::applyStyle()
{
  if (!m_plot)
    return;

  m_plot->setBackground(QBrush(Qt::transparent));

  QPen gridPen(QColor(255, 255, 255, 15));  // ~rgba(1,1,1,.06) * 255
  gridPen.setStyle(Qt::DashLine);
  m_plot->yAxis->grid()->setPen(gridPen);
  m_plot->yAxis->grid()->setZeroLinePen(gridPen);
  m_plot->xAxis->grid()->setPen(gridPen);

  QColor label = QColor(QStringLiteral("#b3b3b3"));
  m_plot->xAxis->setLabelColor(label);
  m_plot->yAxis->setLabelColor(label);
  m_plot->xAxis->setTickLabelColor(label);
  m_plot->yAxis->setTickLabelColor(label);
  m_plot->xAxis->setBasePen(QPen(label));
  m_plot->yAxis->setBasePen(QPen(label));
  m_plot->xAxis->setTickPen(QPen(label));
  m_plot->yAxis->setTickPen(QPen(label));

  QFont f = m_plot->xAxis->tickLabelFont();
  f.setPixelSize(10);
  m_plot->xAxis->setTickLabelFont(f);
  m_plot->yAxis->setTickLabelFont(f);

  m_plot->xAxis->setVisible(m_bottomAxisVisible);
  m_plot->xAxis->setTickLabels(m_bottomAxisVisible);
  if (m_bottomAxisVisible)
    m_plot->xAxis->setLabel(QStringLiteral("Time (s)"));
  else
    m_plot->xAxis->setLabel({});

  if (auto g = m_plot->graph(0))
  {
    QPen ln(m_lineColor);
    ln.setWidthF(1.5);
    ln.setCosmetic(false);
    g->setPen(ln);
    g->setAdaptiveSampling(true);
  }

  const QMargins m(8, 4, 4, m_bottomAxisVisible ? 22 : 4);
  m_plot->plotLayout()->setMargins(m);
}

void QCustomPlotQuickItem::setLineColor(const QColor &c)
{
  if (m_lineColor == c)
    return;
  m_lineColor = c;
  ensurePlot();
  if (m_plot && m_plot->graphCount() > 0)
    m_plot->graph(0)->setPen([&] {
      QPen p(c);
      p.setWidthF(1.5);
      return p;
    }());
  emit lineColorChanged();
  update();
}

void QCustomPlotQuickItem::setBottomAxisVisible(bool v)
{
  if (m_bottomAxisVisible == v)
    return;
  m_bottomAxisVisible = v;
  ensurePlot();
  applyStyle();
  emit bottomAxisVisibleChanged();
  update();
}

void QCustomPlotQuickItem::setSamples(const QVariantList &xs, const QVariantList &ys)
{
  ensurePlot();
  if (!m_plot || xs.size() != ys.size() || xs.isEmpty())
  {
    m_cachedX.clear();
    m_cachedY.clear();
    if (m_plot && m_plot->graphCount() > 0)
      m_plot->graph(0)->data()->clear();
    update();
    return;
  }

  const int n = xs.size();
  m_cachedX.resize(n);
  m_cachedY.resize(n);
  for (int i = 0; i < n; ++i)
  {
    m_cachedX[i] = xs.at(i).toDouble();
    m_cachedY[i] = ys.at(i).toDouble();
  }
  if (m_plot->graphCount() > 0)
    m_plot->graph(0)->setData(m_cachedX, m_cachedY, true);
  update();
}

void QCustomPlotQuickItem::setData(const QVector<double> &xs, const QVector<double> &ys)
{
  ensurePlot();
  if (!m_plot || xs.size() != ys.size() || xs.isEmpty())
  {
    m_cachedX.clear();
    m_cachedY.clear();
    if (m_plot && m_plot->graphCount() > 0)
      m_plot->graph(0)->data()->clear();
    update();
    return;
  }

  m_cachedX = xs;
  m_cachedY = ys;
  if (m_plot->graphCount() > 0)
    m_plot->graph(0)->setData(m_cachedX, m_cachedY, true);
  update();
}

void QCustomPlotQuickItem::setYRange(double yMin, double yMax)
{
  ensurePlot();
  if (!m_plot)
    return;
  m_plot->yAxis->setRange(yMin, yMax);
  update();
}

void QCustomPlotQuickItem::setXRange(double xMin, double xMax)
{
  ensurePlot();
  if (!m_plot)
    return;
  m_plot->xAxis->setRange(xMin, xMax);
  update();
}

void QCustomPlotQuickItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
  QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
  update();
}

void QCustomPlotQuickItem::paint(QPainter *painter)
{
  ensurePlot();
  replotToPainter(painter);
}

void QCustomPlotQuickItem::replotToPainter(QPainter *painter)
{
  if (!m_plot || !painter)
    return;

  const int w = qMax(2, int(std::ceil(width())));
  const int h = qMax(2, int(std::ceil(height())));
  m_plot->resize(w, h);

  // Keep ranges if empty graph
  m_plot->replot(QCustomPlot::rpImmediateRefresh);

  const QPixmap px = m_plot->toPixmap(w, h);
  painter->drawPixmap(0, 0, px);
}
