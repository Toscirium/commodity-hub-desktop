#pragma once

#include <QVector>
#include <QWidget>

#include "models/NewsArticle.h"

class QLabel;
class QVBoxLayout;

// Article list for whichever commodity is currently expanded in the
// watchlist - lives directly below ChartPanel in the same disposable
// container (see WatchlistPanel::showDetailsBelowItem), mirroring
// CommodityCard.tsx's chart-then-news layout in the web app.
class NewsPanel : public QWidget
{
    Q_OBJECT
public:
    explicit NewsPanel(QWidget *parent = nullptr);

    void setCommodityName(const QString &name);
    void setArticles(const QString &commodityName, const QVector<NewsArticle> &articles);
    void setError(const QString &commodityName, const QString &message);

private:
    void clearArticleWidgets();
    void showStatus(const QString &text);
    QWidget *buildArticleCard(const NewsArticle &article) const;

    QString m_commodityName;
    QLabel *m_titleLabel;
    QLabel *m_statusLabel;
    QVBoxLayout *m_articlesLayout;
};
