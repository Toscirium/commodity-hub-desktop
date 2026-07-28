#include "CommodityCardWidget.h"

#include <QColor>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace {
const QColor kPositiveColor(76, 183, 130);
const QColor kNegativeColor(231, 75, 75);
}

CommodityCardWidget::CommodityCardWidget(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("commodityCard"));
    setAttribute(Qt::WA_StyledBackground, true);

    m_iconLabel = new QLabel(QStringLiteral("$"), this);
    m_iconLabel->setObjectName(QStringLiteral("commodityIcon"));
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setFixedSize(40, 40);
    m_iconLabel->setFont(QFont(QStringLiteral("JetBrains Mono"), 14, QFont::Bold));

    m_nameLabel = new QLabel(this);
    m_nameLabel->setObjectName(QStringLiteral("commodityName"));

    m_tickerBadge = new QLabel(this);
    m_tickerBadge->setObjectName(QStringLiteral("pillBadge"));
    m_exchangeBadge = new QLabel(this);
    m_exchangeBadge->setObjectName(QStringLiteral("pillBadge"));

    m_priceLabel = new QLabel(this);
    m_priceLabel->setObjectName(QStringLiteral("commodityPrice"));

    m_changeLabel = new QLabel(this);
    m_changeLabel->setFont(QFont(QStringLiteral("JetBrains Mono"), 11, QFont::DemiBold));

    m_expandIcon = new QLabel(QStringLiteral("▼"), this); // ▼, flips to ▲ when expanded
    m_expandIcon->setStyleSheet(QStringLiteral("QLabel { color: #898d94; }"));
    m_expandIcon->setAlignment(Qt::AlignCenter);
    m_expandIcon->setFixedWidth(20);

    auto *badgeRow = new QHBoxLayout;
    badgeRow->setSpacing(6);
    badgeRow->addWidget(m_nameLabel);
    badgeRow->addWidget(m_tickerBadge);
    badgeRow->addWidget(m_exchangeBadge);
    badgeRow->addStretch(1);

    auto *priceRow = new QHBoxLayout;
    priceRow->setSpacing(10);
    priceRow->addWidget(m_priceLabel);
    priceRow->addWidget(m_changeLabel);
    priceRow->addStretch(1);

    auto *textColumn = new QVBoxLayout;
    textColumn->setSpacing(4);
    textColumn->addLayout(badgeRow);
    textColumn->addLayout(priceRow);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(10);
    layout->addWidget(m_iconLabel);
    layout->addLayout(textColumn, 1);
    layout->addWidget(m_expandIcon);
}

void CommodityCardWidget::setCommodity(const Commodity &commodity)
{
    m_nameLabel->setText(commodity.name);
    m_tickerBadge->setText(commodity.symbol);
    m_tickerBadge->setVisible(!commodity.symbol.isEmpty());
    m_exchangeBadge->setText(commodity.venue);
    m_exchangeBadge->setVisible(!commodity.venue.isEmpty());

    m_priceLabel->setText(QStringLiteral("$%1").arg(commodity.price, 0, 'f', 2));

    const bool positive = commodity.changePercent >= 0.0;
    m_changeLabel->setText(QStringLiteral("%1 %2%3%")
                                .arg(positive ? QStringLiteral("▲") : QStringLiteral("▼"),
                                     positive ? "+" : "", QString::number(commodity.changePercent, 'f', 2)));
    m_changeLabel->setStyleSheet(
        QStringLiteral("QLabel { color: %1; }").arg((positive ? kPositiveColor : kNegativeColor).name()));
}

void CommodityCardWidget::setExpanded(bool expanded)
{
    m_expandIcon->setText(expanded ? QStringLiteral("▲") : QStringLiteral("▼"));
}
