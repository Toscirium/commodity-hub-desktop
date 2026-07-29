#include "CommodityTableModel.h"

#include <QColor>
#include <QFont>

namespace {
const QColor kPositiveColor(76, 183, 130);
const QColor kNegativeColor(231, 75, 75);

QString capitalize(const QString &text)
{
    if (text.isEmpty())
        return text;
    return text.at(0).toUpper() + text.mid(1);
}
}

CommodityTableModel::CommodityTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void CommodityTableModel::setCommodities(const QVector<Commodity> &commodities)
{
    beginResetModel();
    m_commodities = commodities;
    endResetModel();
}

bool CommodityTableModel::isNumericColumn(int column)
{
    return column == ColumnPrice || column == ColumnChange || column == ColumnChangePercent
        || column == ColumnVolume;
}

int CommodityTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_commodities.size();
}

int CommodityTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return ColumnCount;
}

QVariant CommodityTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_commodities.size())
        return QVariant();

    const Commodity &c = m_commodities.at(index.row());
    const int column = index.column();

    if (role == Qt::TextAlignmentRole && isNumericColumn(column))
        return int(Qt::AlignRight | Qt::AlignVCenter);

    if (role == Qt::FontRole && isNumericColumn(column))
        return QFont(QStringLiteral("JetBrains Mono"));

    if (role == Qt::ForegroundRole && (column == ColumnChange || column == ColumnChangePercent))
        return c.changePercent >= 0.0 ? kPositiveColor : kNegativeColor;

    // Raw numeric value for the proxy model's sort comparator — Qt::DisplayRole
    // below is a formatted string, which would sort lexicographically.
    if (role == Qt::EditRole && isNumericColumn(column)) {
        switch (column) {
        case ColumnPrice:
            return c.price;
        case ColumnChange:
            return c.change;
        case ColumnChangePercent:
            return c.changePercent;
        case ColumnVolume:
            return c.volume;
        default:
            return QVariant();
        }
    }

    if (role != Qt::DisplayRole)
        return QVariant();

    switch (column) {
    case ColumnSymbol:
        return c.symbol;
    case ColumnName:
        return c.name;
    case ColumnCategory:
        return capitalize(c.category);
    case ColumnPrice:
        return QString::number(c.price, 'f', 2);
    case ColumnChange: {
        const QString sign = c.change >= 0.0 ? QStringLiteral("+") : QString();
        return sign + QString::number(c.change, 'f', 2);
    }
    case ColumnChangePercent: {
        const QString sign = c.changePercent >= 0.0 ? QStringLiteral("+") : QString();
        return sign + QString::number(c.changePercent, 'f', 2) + QStringLiteral("%");
    }
    case ColumnVolume:
        return c.volume > 0.0 ? QString::number(c.volume, 'f', 0) : QStringLiteral("—");
    default:
        return QVariant();
    }
}

QVariant CommodityTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);

    switch (section) {
    case ColumnSymbol:
        return tr("Symbol");
    case ColumnName:
        return tr("Name");
    case ColumnCategory:
        return tr("Category");
    case ColumnPrice:
        return tr("Price");
    case ColumnChange:
        return tr("Change");
    case ColumnChangePercent:
        return tr("Change %");
    case ColumnVolume:
        return tr("Volume");
    default:
        return QVariant();
    }
}
