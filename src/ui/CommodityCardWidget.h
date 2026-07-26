#pragma once

#include <QWidget>

#include "models/Commodity.h"

class QLabel;

// Card-style row for a single commodity: icon chip, name + ticker/exchange
// pill badges on the first line, big price + unit + colored change% on the
// second. Mirrors the row layout used throughout commodity-hub's mobile UI
// (see public/screenshots/*.png in the commodity-hub repo), in place of a
// dense spreadsheet-style table.
class CommodityCardWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CommodityCardWidget(QWidget *parent = nullptr);

    void setCommodity(const Commodity &commodity);
    // Chevron indicator reflecting whether this card's chart/news are
    // currently shown below it (see WatchlistPanel's click-to-toggle).
    void setExpanded(bool expanded);

private:
    QLabel *m_iconLabel;
    QLabel *m_nameLabel;
    QLabel *m_tickerBadge;
    QLabel *m_exchangeBadge;
    QLabel *m_priceLabel;
    QLabel *m_changeLabel;
    QLabel *m_expandIcon;
};
