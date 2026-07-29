#include "SpreadCalculatorPanel.h"

#include <QColor>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>

#include "core/SpreadFormulas.h"

namespace {
const QColor kPositiveColor(76, 183, 130);
const QColor kNegativeColor(231, 75, 75);
const QColor kNeutralColor(229, 231, 235); // --foreground

QString formatValue(double value, const QString &unit)
{
    return unit == QStringLiteral("ratio") ? QString::number(value, 'f', 3) : QString::number(value, 'f', 2);
}

// One preset per row: name + description on the left, unit + colored value
// on the right — same "card" look the rest of the app uses for list rows.
QWidget *makeSpreadRow(const SpreadPreset &preset, const std::optional<double> &value)
{
    auto *row = new QWidget;
    row->setObjectName(QStringLiteral("card"));
    row->setAttribute(Qt::WA_StyledBackground, true);

    auto *nameLabel = new QLabel(preset.name, row);
    nameLabel->setStyleSheet(QStringLiteral("font-family:'Space Grotesk';font-size:14px;font-weight:700;"));
    auto *descLabel = new QLabel(preset.description, row);
    descLabel->setStyleSheet(QStringLiteral("color:#898d94;font-size:11px;"));
    descLabel->setWordWrap(true);

    auto *textColumn = new QVBoxLayout;
    textColumn->setSpacing(2);
    textColumn->addWidget(nameLabel);
    textColumn->addWidget(descLabel);

    auto *unitLabel = new QLabel(preset.unit, row);
    unitLabel->setStyleSheet(QStringLiteral("color:#898d94;font-size:11px;"));
    unitLabel->setAlignment(Qt::AlignRight);

    QColor valueColor = kNeutralColor;
    if (value && preset.unit != QStringLiteral("ratio"))
        valueColor = *value >= 0.0 ? kPositiveColor : kNegativeColor;
    auto *valueLabel =
        new QLabel(value ? formatValue(*value, preset.unit) : QStringLiteral("—"), row);
    valueLabel->setStyleSheet(QStringLiteral("font-family:'JetBrains Mono';font-size:18px;font-weight:700;color:%1;")
                                   .arg(valueColor.name()));
    valueLabel->setAlignment(Qt::AlignRight);

    auto *valueColumn = new QVBoxLayout;
    valueColumn->setSpacing(2);
    valueColumn->addWidget(unitLabel);
    valueColumn->addWidget(valueLabel);

    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(14, 10, 14, 10);
    layout->setSpacing(12);
    layout->addLayout(textColumn, 1);
    layout->addLayout(valueColumn);

    return row;
}
}

SpreadCalculatorPanel::SpreadCalculatorPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *iconTile = new QLabel(QStringLiteral("\U000021C4"), this); // ⇄, matches the sidebar's placeholder icon
    iconTile->setObjectName(QStringLiteral("logoTile"));
    iconTile->setAlignment(Qt::AlignCenter);
    iconTile->setFixedSize(36, 36);

    auto *eyebrow = new QLabel(tr("TOOLS"), this);
    eyebrow->setObjectName(QStringLiteral("panelHeading"));
    auto *heading = new QLabel(tr("Spread Calculator"), this);
    heading->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(tr("Live commodity spread and ratio benchmarks"), this);
    subtitle->setObjectName(QStringLiteral("pageSubtitle"));

    auto *titleColumn = new QVBoxLayout;
    titleColumn->setSpacing(2);
    titleColumn->addWidget(eyebrow);
    titleColumn->addWidget(heading);
    titleColumn->addWidget(subtitle);

    auto *headerRow = new QHBoxLayout;
    headerRow->setSpacing(12);
    headerRow->addWidget(iconTile);
    headerRow->addLayout(titleColumn, 1);

    m_list = new QListWidget(this);
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setSpacing(8);
    m_list->setSelectionMode(QAbstractItemView::NoSelection);
    m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    m_emptyLabel = new QLabel(tr("Waiting for market data…"), this);
    m_emptyLabel->setObjectName(QStringLiteral("emptyState"));
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setVisible(false);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);
    layout->addLayout(headerRow);
    layout->addWidget(m_list, 1);
    layout->addWidget(m_emptyLabel);

    refreshSpreads();
}

void SpreadCalculatorPanel::setCommodities(const QVector<Commodity> &commodities)
{
    m_commodities = commodities;
    refreshSpreads();
}

void SpreadCalculatorPanel::refreshSpreads()
{
    m_list->clear();

    const bool empty = m_commodities.isEmpty();
    m_emptyLabel->setVisible(empty);
    m_list->setVisible(!empty);
    if (empty)
        return;

    for (const SpreadPreset &preset : spreadPresets()) {
        const std::optional<double> value = computeSpread(preset, m_commodities);
        auto *item = new QListWidgetItem(m_list);
        auto *row = makeSpreadRow(preset, value);
        item->setSizeHint(row->sizeHint());
        item->setFlags(Qt::NoItemFlags);
        m_list->addItem(item);
        m_list->setItemWidget(item, row);
    }
}
