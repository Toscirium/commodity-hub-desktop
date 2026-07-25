#include "ChartPanel.h"

#include <QCandlestickSeries>
#include <QCandlestickSet>
#include <QChart>
#include <QChartView>
#include <QComboBox>
#include <QDateTimeAxis>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineSeries>
#include <QPainter>
#include <QStandardItemModel>
#include <QValueAxis>
#include <QVBoxLayout>
#include <algorithm>
#include <limits>

namespace {
// commodity-hub dark-theme tokens (see resources/theme.qss for the rest).
const QColor kCardColor(0x10, 0x11, 0x13);
const QColor kPrimaryColor(0x7c, 0x3b, 0xed);
const QColor kBorderColor(0x27, 0x28, 0x2b);
const QColor kGridColor(0x1d, 0x1e, 0x20);
const QColor kMutedForeground(0x89, 0x8d, 0x94);
const QColor kPositiveColor(0x4c, 0xb7, 0x82);
const QColor kNegativeColor(0xe7, 0x4b, 0x4b);

constexpr int ChartTypeCandlestickIndex = 1;
}

ChartPanel::ChartPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("card"));
    setAttribute(Qt::WA_StyledBackground, true);

    m_titleLabel = new QLabel(tr("Select a commodity"), this);
    m_titleLabel->setObjectName("chartTitle");

    m_timeframeCombo = new QComboBox(this);
    m_timeframeCombo->addItem(tr("1D"), QStringLiteral("1d"));
    m_timeframeCombo->addItem(tr("1M"), QStringLiteral("1m"));
    m_timeframeCombo->addItem(tr("3M"), QStringLiteral("3m"));
    m_timeframeCombo->addItem(tr("6M"), QStringLiteral("6m"));
    m_timeframeCombo->addItem(tr("1Y"), QStringLiteral("1y"));
    m_timeframeCombo->addItem(tr("2Y"), QStringLiteral("2y"));
    m_timeframeCombo->setCurrentIndex(2);

    m_chartTypeCombo = new QComboBox(this);
    m_chartTypeCombo->addItem(tr("Line"), QStringLiteral("line"));
    m_chartTypeCombo->addItem(tr("Candlestick"), QStringLiteral("candlestick"));

    m_series = new QLineSeries(this);
    m_series->setPen(QPen(kPrimaryColor, 2));

    m_candlestickSeries = new QCandlestickSeries(this);
    m_candlestickSeries->setIncreasingColor(kPositiveColor);
    m_candlestickSeries->setDecreasingColor(kNegativeColor);
    m_candlestickSeries->setBodyOutlineVisible(false);
    m_candlestickSeries->setVisible(false);

    m_axisX = new QDateTimeAxis(this);
    m_axisX->setFormat("MMM d");
    m_axisX->setLabelsColor(kMutedForeground);
    m_axisX->setLinePen(QPen(kBorderColor));
    m_axisX->setGridLinePen(QPen(kGridColor));

    m_axisY = new QValueAxis(this);
    m_axisY->setLabelFormat("%.2f");
    m_axisY->setLabelsColor(kMutedForeground);
    m_axisY->setLinePen(QPen(kBorderColor));
    m_axisY->setGridLinePen(QPen(kGridColor));

    m_chart = new QChart();
    m_chart->addSeries(m_series);
    m_chart->addSeries(m_candlestickSeries);
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_series->attachAxis(m_axisX);
    m_series->attachAxis(m_axisY);
    m_candlestickSeries->attachAxis(m_axisX);
    m_candlestickSeries->attachAxis(m_axisY);
    m_chart->legend()->hide();
    m_chart->setBackgroundBrush(QBrush(kCardColor));
    m_chart->setBackgroundPen(QPen(Qt::NoPen));
    m_chart->setMargins(QMargins(4, 8, 12, 4));

    m_chartView = new QChartView(m_chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setBackgroundBrush(QBrush(kCardColor));
    m_chartView->setFrameShape(QFrame::NoFrame);

    auto *headerRow = new QHBoxLayout;
    headerRow->addWidget(m_titleLabel, 1);
    headerRow->addWidget(m_chartTypeCombo);
    headerRow->addWidget(m_timeframeCombo);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);
    layout->addLayout(headerRow);
    layout->addWidget(m_chartView, 1);

    connect(m_timeframeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ChartPanel::onTimeframeChanged);
    connect(m_chartTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ChartPanel::onChartTypeChanged);
}

void ChartPanel::setCommodityName(const QString &name)
{
    m_commodityName = name;
    m_titleLabel->setText(name);
    m_series->clear();
    m_candlestickSeries->clear();
}

void ChartPanel::setBars(const QString &commodityName, const QVector<OhlcBar> &bars, bool ohlcAvailable)
{
    if (commodityName != m_commodityName)
        return;

    m_series->clear();
    m_candlestickSeries->clear();

    m_ohlcAvailable = ohlcAvailable;
    applyChartTypeAvailability();

    // '1D' bars are now real sub-daily (5min) candles from Massive, so the
    // axis should show time-of-day rather than a date that repeats per bar.
    const bool intraday = m_timeframeCombo->currentData().toString() == QStringLiteral("1d");
    m_axisX->setFormat(intraday ? QStringLiteral("h:mm ap") : QStringLiteral("MMM d"));

    if (bars.isEmpty())
        return;

    double minPrice = std::numeric_limits<double>::max();
    double maxPrice = std::numeric_limits<double>::lowest();
    qint64 minMs = std::numeric_limits<qint64>::max();
    qint64 maxMs = std::numeric_limits<qint64>::min();

    for (const OhlcBar &bar : bars) {
        QDateTime dt = QDateTime::fromString(bar.date, Qt::ISODate);
        if (!dt.isValid())
            dt = QDateTime::fromString(bar.date, "yyyy-MM-dd");
        if (!dt.isValid())
            continue;

        const qint64 ms = dt.toMSecsSinceEpoch();
        m_series->append(static_cast<double>(ms), bar.close);
        m_candlestickSeries->append(
            new QCandlestickSet(bar.open, bar.high, bar.low, bar.close, static_cast<double>(ms)));

        minPrice = std::min({minPrice, bar.low, bar.close});
        maxPrice = std::max({maxPrice, bar.high, bar.close});
        minMs = std::min(minMs, ms);
        maxMs = std::max(maxMs, ms);
    }

    if (minMs > maxMs)
        return;

    const double padding = (maxPrice - minPrice) * 0.05;
    m_axisY->setRange(minPrice - padding, maxPrice + padding);
    m_axisX->setRange(QDateTime::fromMSecsSinceEpoch(minMs), QDateTime::fromMSecsSinceEpoch(maxMs));
}

void ChartPanel::applyChartTypeAvailability()
{
    auto *model = qobject_cast<QStandardItemModel *>(m_chartTypeCombo->model());
    if (model) {
        if (QStandardItem *item = model->item(ChartTypeCandlestickIndex))
            item->setEnabled(m_ohlcAvailable);
    }

    if (!m_ohlcAvailable && m_chartTypeCombo->currentIndex() == ChartTypeCandlestickIndex) {
        m_chartTypeCombo->blockSignals(true);
        m_chartTypeCombo->setCurrentIndex(0);
        m_chartTypeCombo->blockSignals(false);
        m_series->setVisible(true);
        m_candlestickSeries->setVisible(false);
    }
}

void ChartPanel::onTimeframeChanged(int index)
{
    emit timeframeChanged(m_timeframeCombo->itemData(index).toString());
}

void ChartPanel::onChartTypeChanged(int index)
{
    const bool wantsCandlestick = index == ChartTypeCandlestickIndex && m_ohlcAvailable;
    m_series->setVisible(!wantsCandlestick);
    m_candlestickSeries->setVisible(wantsCandlestick);
}
