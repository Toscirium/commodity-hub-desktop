#pragma once

#include <QAbstractTableModel>
#include <QVector>

#include "models/PriceAlert.h"

class AlertsTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit AlertsTableModel(QObject *parent = nullptr);

    void setAlerts(const QVector<PriceAlert> &alerts);
    const PriceAlert &alertAt(int row) const { return m_alerts.at(row); }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

signals:
    void activeToggled(const QString &alertId, bool active);

private:
    QVector<PriceAlert> m_alerts;
};
