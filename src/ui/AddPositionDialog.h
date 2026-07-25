#pragma once

#include <QDialog>
#include <QVector>

#include "models/Commodity.h"
#include "models/PortfolioPosition.h"

class QComboBox;
class QDateEdit;
class QLineEdit;

class AddPositionDialog : public QDialog
{
    Q_OBJECT
public:
    // Pass an existing position to edit it in place (title/button switch to
    // "Edit"/"Save" and the fields are pre-filled); leave it default for the
    // "Add Position" flow.
    explicit AddPositionDialog(const QVector<Commodity> &commodities, QWidget *parent = nullptr,
                                const PortfolioPosition &existing = PortfolioPosition());

    QString commodityName() const;
    double quantity() const;
    double entryPrice() const;
    QString entryDate() const;
    QString notes() const;

private slots:
    void onCommodityChanged(int index);
    void onAccept();

private:
    QComboBox *m_commodityCombo;
    QLineEdit *m_quantityEdit;
    QLineEdit *m_entryPriceEdit;
    QDateEdit *m_entryDateEdit;
    QLineEdit *m_notesEdit;
};
