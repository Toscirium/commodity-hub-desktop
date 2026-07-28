#include "AlertsPanel.h"

#include <QColor>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

#include "AddAlertDialog.h"
#include "AlertsTableModel.h"

AlertsPanel::AlertsPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *heading = new QLabel(tr("Price Alerts"), this);
    heading->setObjectName(QStringLiteral("pageTitle"));
    auto *subtitle = new QLabel(tr("Get notified when a commodity crosses a price you care about"), this);
    subtitle->setObjectName(QStringLiteral("pageSubtitle"));
    subtitle->setWordWrap(true);

    m_model = new AlertsTableModel(this);
    m_tableView = new QTableView(this);
    m_tableView->setModel(m_model);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setEditTriggers(QAbstractItemView::AllEditTriggers);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->verticalHeader()->setVisible(false);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setShowGrid(false);
    m_tableView->setFrameShape(QFrame::NoFrame);
    m_tableView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_tableView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    m_emptyLabel = new QLabel(tr("No price alerts yet — click “New Alert” to get notified when a commodity "
                                  "crosses a price you care about."),
                               this);
    m_emptyLabel->setObjectName(QStringLiteral("emptyState"));
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->setWordWrap(true);
    m_emptyLabel->hide();

    m_newAlertButton = new QPushButton(tr("New Alert"), this);
    m_deleteButton = new QPushButton(tr("Delete"), this);
    m_deleteButton->setObjectName(QStringLiteral("destructiveButton"));
    m_deleteButton->setEnabled(false);

    m_triggersHeading = new QLabel(tr("Recent Triggers"), this);
    m_triggersHeading->setObjectName(QStringLiteral("panelHeading"));
    m_triggersList = new QListWidget(this);
    m_triggersList->setFrameShape(QFrame::NoFrame);
    m_triggersList->setMaximumHeight(140);
    m_triggersList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    auto *buttonRow = new QHBoxLayout;
    buttonRow->addWidget(m_newAlertButton);
    buttonRow->addWidget(m_deleteButton);
    buttonRow->addStretch(1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(8);
    layout->addWidget(heading);
    layout->addWidget(subtitle);
    layout->addWidget(m_tableView, 1);
    layout->addWidget(m_emptyLabel, 1);
    layout->addLayout(buttonRow);
    layout->addWidget(m_triggersHeading);
    layout->addWidget(m_triggersList);

    connect(m_tableView->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            [this]() { m_deleteButton->setEnabled(m_tableView->currentIndex().isValid()); });
    connect(m_newAlertButton, &QPushButton::clicked, this, &AlertsPanel::onNewAlertClicked);
    connect(m_deleteButton, &QPushButton::clicked, this, &AlertsPanel::onDeleteClicked);
    connect(m_model, &AlertsTableModel::activeToggled, this, &AlertsPanel::alertActiveToggled);
}

void AlertsPanel::setCommodities(const QVector<Commodity> &commodities)
{
    m_commodities = commodities;
}

void AlertsPanel::setAlerts(const QVector<PriceAlert> &alerts)
{
    m_model->setAlerts(alerts);
    m_deleteButton->setEnabled(false);

    m_tableView->setVisible(!alerts.isEmpty());
    m_emptyLabel->setVisible(alerts.isEmpty());
}

void AlertsPanel::setTriggers(const QVector<PriceAlertTrigger> &triggers)
{
    m_triggers = triggers;
    m_triggersHeading->setText(triggers.isEmpty() ? tr("Recent Triggers")
                                                    : tr("Recent Triggers (%1)").arg(triggers.size()));
    rebuildTriggerList();
}

void AlertsPanel::rebuildTriggerList()
{
    m_triggersList->clear();

    if (m_triggers.isEmpty()) {
        auto *item = new QListWidgetItem(tr("No recent triggers"), m_triggersList);
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        item->setForeground(QColor(137, 141, 148)); // --muted-foreground
        m_triggersList->addItem(item);
        return;
    }

    for (const PriceAlertTrigger &trigger : m_triggers) {
        auto *row = new QWidget(m_triggersList);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(8, 4, 8, 4);

        const QString condition = trigger.condition == QStringLiteral("above") ? tr("above") : tr("below");
        auto *label = new QLabel(tr("%1 went %2 %3 (now %4)")
                                      .arg(trigger.commodityName, condition,
                                           QString::number(trigger.targetPrice, 'f', 2),
                                           QString::number(trigger.triggeredPrice, 'f', 2)),
                                  row);
        label->setWordWrap(true);

        auto *dismissButton = new QPushButton(tr("Dismiss"), row);
        dismissButton->setObjectName(QStringLiteral("secondaryButton"));
        const QString triggerId = trigger.id;
        connect(dismissButton, &QPushButton::clicked, this,
                [this, triggerId]() { emit dismissTriggerRequested(triggerId); });

        rowLayout->addWidget(label, 1);
        rowLayout->addWidget(dismissButton);

        auto *item = new QListWidgetItem(m_triggersList);
        item->setSizeHint(row->sizeHint());
        m_triggersList->addItem(item);
        m_triggersList->setItemWidget(item, row);
    }
}

void AlertsPanel::onNewAlertClicked()
{
    if (m_commodities.isEmpty())
        return;

    AddAlertDialog dialog(m_commodities, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    emit createAlertRequested(dialog.commodityName(), dialog.commoditySymbol(), dialog.condition(),
                               dialog.targetPrice(), dialog.note());
}

void AlertsPanel::onDeleteClicked()
{
    const QModelIndex index = m_tableView->currentIndex();
    if (!index.isValid())
        return;

    emit deleteAlertRequested(m_model->alertAt(index.row()).id);
}
