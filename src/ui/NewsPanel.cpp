#include "NewsPanel.h"

#include <QDateTime>
#include <QFrame>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

namespace {
const QString kMutedForeground = QStringLiteral("#898d94");
const QString kBorderColor = QStringLiteral("#27282b");

QString formatTimeAgo(const QString &isoDate)
{
    const QDateTime published = QDateTime::fromString(isoDate, Qt::ISODate);
    if (!published.isValid())
        return QString();

    const qint64 diffSecs = published.secsTo(QDateTime::currentDateTimeUtc());
    if (diffSecs < 3600)
        return QObject::tr("Now");
    if (diffSecs < 86400)
        return QObject::tr("%1h ago").arg(diffSecs / 3600);
    return QObject::tr("%1d ago").arg(diffSecs / 86400);
}
}

NewsPanel::NewsPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("card"));
    setAttribute(Qt::WA_StyledBackground, true);

    m_titleLabel = new QLabel(tr("NEWS"), this);
    m_titleLabel->setObjectName(QStringLiteral("panelHeading"));

    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet(QStringLiteral("QLabel { color: %1; }").arg(kMutedForeground));
    m_statusLabel->setWordWrap(true);

    auto *articlesContainer = new QWidget;
    articlesContainer->setAttribute(Qt::WA_StyledBackground, false);
    m_articlesLayout = new QVBoxLayout(articlesContainer);
    m_articlesLayout->setContentsMargins(0, 0, 0, 0);
    m_articlesLayout->setSpacing(14);

    // Fixed-height scroll region rather than letting the article list grow
    // this widget arbitrarily tall: the details row it lives in (see
    // WatchlistPanel::showDetailsBelowItem) gets one fixed QListWidgetItem
    // size hint, not a size that re-flows with content.
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setFixedHeight(260);
    scrollArea->setWidget(articlesContainer);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);
    layout->addWidget(m_titleLabel);
    layout->addWidget(m_statusLabel);
    layout->addWidget(scrollArea);

    showStatus(tr("Select a commodity to see related news."));
}

void NewsPanel::setCommodityName(const QString &name)
{
    m_commodityName = name;
    clearArticleWidgets();
    showStatus(tr("Loading news…"));
}

void NewsPanel::setArticles(const QString &commodityName, const QVector<NewsArticle> &articles)
{
    if (commodityName != m_commodityName)
        return;

    clearArticleWidgets();

    if (articles.isEmpty()) {
        showStatus(tr("No recent news found for %1.").arg(commodityName));
        return;
    }

    m_statusLabel->hide();
    for (const NewsArticle &article : articles)
        m_articlesLayout->addWidget(buildArticleCard(article));
    m_articlesLayout->addStretch(1);
}

void NewsPanel::setError(const QString &commodityName, const QString &message)
{
    if (commodityName != m_commodityName)
        return;

    clearArticleWidgets();
    showStatus(tr("Couldn't load news: %1").arg(message));
}

void NewsPanel::clearArticleWidgets()
{
    while (QLayoutItem *item = m_articlesLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
}

void NewsPanel::showStatus(const QString &text)
{
    m_statusLabel->setText(text);
    m_statusLabel->show();
}

QWidget *NewsPanel::buildArticleCard(const NewsArticle &article) const
{
    auto *card = new QWidget;
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    auto *titleLabel = new QLabel(card);
    titleLabel->setTextFormat(Qt::RichText);
    titleLabel->setOpenExternalLinks(true);
    titleLabel->setWordWrap(true);
    const QString title = article.title.isEmpty() ? tr("(untitled)") : article.title.toHtmlEscaped();
    if (!article.url.isEmpty())
        titleLabel->setText(QStringLiteral("<a href=\"%1\" style=\"color:#e5e7eb; text-decoration:none;\">%2</a>")
                                 .arg(article.url.toHtmlEscaped(), title));
    else
        titleLabel->setText(title);
    titleLabel->setStyleSheet(QStringLiteral("QLabel { font-weight: 600; font-size: 13px; }"));

    QStringList metaParts;
    if (!article.source.isEmpty())
        metaParts << article.source;
    const QString timeAgo = formatTimeAgo(article.publishedAt);
    if (!timeAgo.isEmpty())
        metaParts << timeAgo;

    auto *metaLabel = new QLabel(metaParts.join(QStringLiteral("  ·  ")), card);
    metaLabel->setStyleSheet(QStringLiteral("QLabel { color: %1; font-size: 11px; }").arg(kMutedForeground));

    layout->addWidget(titleLabel);
    layout->addWidget(metaLabel);

    if (!article.description.isEmpty()) {
        auto *descriptionLabel = new QLabel(article.description, card);
        descriptionLabel->setWordWrap(true);
        descriptionLabel->setStyleSheet(QStringLiteral("QLabel { color: %1; }").arg(kMutedForeground));
        layout->addWidget(descriptionLabel);
    }

    auto *separator = new QFrame(card);
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet(QStringLiteral("QFrame { color: %1; }").arg(kBorderColor));
    layout->addWidget(separator);

    return card;
}
