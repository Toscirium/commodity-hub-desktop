#include "PortfolioTableModel.h"

#include <QColor>
#include <QFont>

namespace {
constexpr int ColumnCommodity = 0;
constexpr int ColumnQuantity = 1;
constexpr int ColumnEntryPrice = 2;
constexpr int ColumnCurrentPrice = 3;
constexpr int ColumnValue = 4;
constexpr int ColumnReturn = 5;
constexpr int ColumnReturnPercent = 6;
constexpr int ColumnCount = 7;

const QColor kPositiveColor(76, 183, 130);
const QColor kNegativeColor(231, 75, 75);

double entryValue(const PortfolioTableModel::Row &row)
{
    return row.position.quantity * row.position.entryPrice;
}

double currentValue(const PortfolioTableModel::Row &row)
{
    return row.position.quantity * row.currentPrice;
}

double totalReturn(const PortfolioTableModel::Row &row)
{
    return currentValue(row) - entryValue(row);
}
}

PortfolioTableModel::PortfolioTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void PortfolioTableModel::setRows(const QVector<Row> &rows)
{
    beginResetModel();
    m_rows = rows;
    endResetModel();
}

int PortfolioTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_rows.size();
}

int PortfolioTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return ColumnCount;
}

QVariant PortfolioTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return QVariant();

    const Row &row = m_rows.at(index.row());
    const bool numericColumn = index.column() >= ColumnQuantity;

    if (role == Qt::TextAlignmentRole && numericColumn)
        return int(Qt::AlignRight | Qt::AlignVCenter);

    if (role == Qt::FontRole && numericColumn)
        return QFont(QStringLiteral("JetBrains Mono"));

    if (role == Qt::ForegroundRole && (index.column() == ColumnReturn || index.column() == ColumnReturnPercent)) {
        if (!row.priceKnown)
            return QVariant();
        return totalReturn(row) >= 0.0 ? kPositiveColor : kNegativeColor;
    }

    if (role != Qt::DisplayRole)
        return QVariant();

    switch (index.column()) {
    case ColumnCommodity:
        return row.position.commodityName;
    case ColumnQuantity:
        return QString::number(row.position.quantity, 'f', row.position.quantity == int(row.position.quantity) ? 0 : 4);
    case ColumnEntryPrice:
        return QString::number(row.position.entryPrice, 'f', 2);
    case ColumnCurrentPrice:
        return row.priceKnown ? QString::number(row.currentPrice, 'f', 2) : QStringLiteral("—");
    case ColumnValue:
        return row.priceKnown ? QString::number(currentValue(row), 'f', 2) : QStringLiteral("—");
    case ColumnReturn: {
        if (!row.priceKnown)
            return QStringLiteral("—");
        const double value = totalReturn(row);
        return QString("%1%2").arg(value >= 0.0 ? "+" : "").arg(value, 0, 'f', 2);
    }
    case ColumnReturnPercent: {
        if (!row.priceKnown || row.position.entryPrice == 0.0)
            return QStringLiteral("—");
        const double pct = (row.currentPrice - row.position.entryPrice) / row.position.entryPrice * 100.0;
        return QString("%1%2%").arg(pct >= 0.0 ? "+" : "").arg(pct, 0, 'f', 2);
    }
    default:
        return QVariant();
    }
}

QVariant PortfolioTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);

    switch (section) {
    case ColumnCommodity:
        return tr("Commodity");
    case ColumnQuantity:
        return tr("Qty");
    case ColumnEntryPrice:
        return tr("Entry");
    case ColumnCurrentPrice:
        return tr("Price");
    case ColumnValue:
        return tr("Value");
    case ColumnReturn:
        return tr("Return");
    case ColumnReturnPercent:
        return tr("Return %");
    default:
        return QVariant();
    }
}
