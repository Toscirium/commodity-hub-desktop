#include "CategoryHeaderWidget.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>

CategoryHeaderWidget::CategoryHeaderWidget(const QString &categoryLabel, int count, QWidget *parent)
    : QWidget(parent)
{
    auto *accentBar = new QFrame(this);
    accentBar->setObjectName(QStringLiteral("categoryAccentBar"));
    accentBar->setFixedWidth(3);

    auto *label = new QLabel(categoryLabel.toUpper(), this);
    label->setObjectName(QStringLiteral("panelHeading"));

    auto *countBadge = new QLabel(QString::number(count), this);
    countBadge->setObjectName(QStringLiteral("pillBadge"));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 8, 0, 4);
    layout->setSpacing(8);
    layout->addWidget(accentBar);
    layout->addWidget(label);
    layout->addWidget(countBadge);
    layout->addStretch(1);
}
