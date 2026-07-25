#pragma once

#include <QDialog>
#include <QVector>

#include "models/Commodity.h"

class QComboBox;
class QLineEdit;

class AddAlertDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddAlertDialog(const QVector<Commodity> &commodities, QWidget *parent = nullptr);

    QString commodityName() const;
    QString commoditySymbol() const;
    QString condition() const;
    double targetPrice() const;
    QString note() const;

private slots:
    void onCommodityChanged(int index);
    void onAccept();

private:
    QVector<Commodity> m_commodities;
    QComboBox *m_commodityCombo;
    QComboBox *m_conditionCombo;
    QLineEdit *m_targetPriceEdit;
    QLineEdit *m_noteEdit;
};
