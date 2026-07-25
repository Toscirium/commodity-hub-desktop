#pragma once

#include <QWidget>

class QLabel;

// Section header above a group of CommodityCardWidget rows: colored accent
// bar + uppercase category name + item count pill, mirroring commodity-hub's
// "CRUDE OIL BENCHMARKS  2" style category headers.
class CategoryHeaderWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CategoryHeaderWidget(const QString &categoryLabel, int count, QWidget *parent = nullptr);
};
