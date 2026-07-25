#pragma once

#include <QWidget>
#include <QVector>

#include "models/OhlcBar.h"

class QLabel;
class QComboBox;
class QChart;
class QChartView;
class QLineSeries;
class QCandlestickSeries;
class QDateTimeAxis;
class QValueAxis;

class ChartPanel : public QWidget
{
    Q_OBJECT
public:
    explicit ChartPanel(QWidget *parent = nullptr);

    void setCommodityName(const QString &name);
    void setBars(const QString &commodityName, const QVector<OhlcBar> &bars, bool ohlcAvailable);

signals:
    void timeframeChanged(const QString &timeframe);

private slots:
    void onTimeframeChanged(int index);
    void onChartTypeChanged(int index);

private:
    void applyChartTypeAvailability();

    QString m_commodityName;
    bool m_ohlcAvailable = true;

    QLabel *m_titleLabel;
    QComboBox *m_timeframeCombo;
    QComboBox *m_chartTypeCombo;
    QChartView *m_chartView;
    QChart *m_chart;
    QLineSeries *m_series;
    QCandlestickSeries *m_candlestickSeries;
    QDateTimeAxis *m_axisX;
    QValueAxis *m_axisY;
};
