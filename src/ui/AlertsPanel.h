#pragma once

#include <QWidget>
#include <QVector>

#include "models/Commodity.h"
#include "models/PriceAlert.h"

class QLabel;
class QListWidget;
class QPushButton;
class QTableView;
class AlertsTableModel;

class AlertsPanel : public QWidget
{
    Q_OBJECT
public:
    explicit AlertsPanel(QWidget *parent = nullptr);

    void setCommodities(const QVector<Commodity> &commodities);
    void setAlerts(const QVector<PriceAlert> &alerts);
    void setTriggers(const QVector<PriceAlertTrigger> &triggers);

signals:
    void createAlertRequested(const QString &commodityName, const QString &commoditySymbol,
                               const QString &condition, double targetPrice, const QString &note);
    void deleteAlertRequested(const QString &alertId);
    void alertActiveToggled(const QString &alertId, bool active);
    void dismissTriggerRequested(const QString &triggerId);

private:
    void onNewAlertClicked();
    void onDeleteClicked();
    void rebuildTriggerList();

    QVector<Commodity> m_commodities;
    QVector<PriceAlertTrigger> m_triggers;

    QTableView *m_tableView;
    QLabel *m_emptyLabel;
    AlertsTableModel *m_model;
    QPushButton *m_newAlertButton;
    QPushButton *m_deleteButton;
    QLabel *m_triggersHeading;
    QListWidget *m_triggersList;
};
