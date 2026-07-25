#include "AlertsTableModel.h"

#include <QFont>

namespace {
constexpr int ColumnCommodity = 0;
constexpr int ColumnCondition = 1;
constexpr int ColumnTarget = 2;
constexpr int ColumnActive = 3;
constexpr int ColumnLastTriggered = 4;
constexpr int ColumnCount = 5;
}

AlertsTableModel::AlertsTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void AlertsTableModel::setAlerts(const QVector<PriceAlert> &alerts)
{
    beginResetModel();
    m_alerts = alerts;
    endResetModel();
}

int AlertsTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_alerts.size();
}

int AlertsTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return ColumnCount;
}

QVariant AlertsTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_alerts.size())
        return QVariant();

    const PriceAlert &alert = m_alerts.at(index.row());

    if (role == Qt::CheckStateRole && index.column() == ColumnActive)
        return alert.isActive ? Qt::Checked : Qt::Unchecked;

    if (role == Qt::TextAlignmentRole && index.column() == ColumnTarget)
        return int(Qt::AlignRight | Qt::AlignVCenter);

    if (role == Qt::FontRole && index.column() == ColumnTarget)
        return QFont(QStringLiteral("JetBrains Mono"));

    if (role != Qt::DisplayRole)
        return QVariant();

    switch (index.column()) {
    case ColumnCommodity:
        return alert.commodityName;
    case ColumnCondition:
        return alert.condition == QStringLiteral("above") ? tr("Above") : tr("Below");
    case ColumnTarget:
        return QString::number(alert.targetPrice, 'f', 2);
    case ColumnActive:
        return QVariant();
    case ColumnLastTriggered:
        return alert.lastTriggeredAt.isEmpty() ? tr("Never") : alert.lastTriggeredAt.left(16).replace('T', ' ');
    default:
        return QVariant();
    }
}

QVariant AlertsTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);

    switch (section) {
    case ColumnCommodity:
        return tr("Commodity");
    case ColumnCondition:
        return tr("Condition");
    case ColumnTarget:
        return tr("Target");
    case ColumnActive:
        return tr("Active");
    case ColumnLastTriggered:
        return tr("Last Triggered");
    default:
        return QVariant();
    }
}

Qt::ItemFlags AlertsTableModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags baseFlags = QAbstractTableModel::flags(index);
    if (index.column() == ColumnActive)
        baseFlags |= Qt::ItemIsUserCheckable;
    return baseFlags;
}

bool AlertsTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::CheckStateRole || index.column() != ColumnActive || !index.isValid())
        return false;

    const bool active = value.toInt() == Qt::Checked;
    emit activeToggled(m_alerts.at(index.row()).id, active);
    return true;
}
