// Integration test that hits commodity-hub's *real* production API (the same
// `fetch-all-commodities` Supabase edge function the shipped app calls, via
// the same CommodityService/SupabaseClient production code, unauthenticated —
// i.e. exactly what an anonymous user's client sends) and runs the response
// through the actual parsing/computation code: Commodity::fromJson,
// computeSpread, CommodityTableModel, CommodityFilterProxyModel. Catches
// regressions a synthetic-fixture unit test can't: API schema drift, and
// real-world data shapes (missing legs, wildly different magnitudes across
// instruments) that hand-built test data tends to accidentally avoid.
//
// Deliberately not built on assumptions about which specific instruments the
// live catalog contains — that's expected to drift — only on invariants that
// must hold for *whatever* it currently returns.

#include <QCoreApplication>
#include <QDebug>
#include <QEventLoop>
#include <QSet>
#include <QTimer>
#include <algorithm>

#include "core/SpreadFormulas.h"
#include "core/Session.h"
#include "core/SupabaseClient.h"
#include "services/CommodityService.h"
#include "ui/CommodityTableModel.h"
#include "ui/ScreenerPanel.h"

namespace {

int g_failures = 0;

void check(bool condition, const QString &description)
{
    if (condition) {
        qInfo().noquote() << "  [PASS]" << description;
    } else {
        qWarning().noquote() << "  [FAIL]" << description;
        ++g_failures;
    }
}

}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    Session session; // never load()ed: isValid() == false, so SupabaseClient
                      // falls back to the anon key, matching a real logged-out user.
    SupabaseClient client(session);
    CommodityService service(client);

    QVector<Commodity> commodities;
    bool loaded = false;
    QString loadError;

    QObject::connect(&service, &CommodityService::commoditiesLoaded, &app,
                      [&](const QVector<Commodity> &result) {
                          commodities = result;
                          loaded = true;
                          QCoreApplication::instance()->exit(0);
                      });
    QObject::connect(&service, &CommodityService::errorOccurred, &app, [&](const QString &error) {
        loadError = error;
        QCoreApplication::instance()->exit(1);
    });

    qInfo().noquote() << "Fetching live commodities from the real fetch-all-commodities edge function...";
    service.fetchAll();

    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    QObject::connect(&timeoutTimer, &QTimer::timeout, &app, [&]() { QCoreApplication::instance()->exit(2); });
    timeoutTimer.start(30'000);

    const int loopResult = app.exec();

    if (loopResult == 2) {
        qWarning() << "FAIL: no response from the live API within 30s";
        return 1;
    }
    if (!loaded) {
        qWarning() << "FAIL: request failed:" << loadError;
        return 1;
    }

    qInfo().noquote() << QStringLiteral("\n=== %1 live commodities parsed via Commodity::fromJson ===").arg(commodities.size());
    for (const Commodity &c : commodities)
        qInfo().noquote() << " " << c.symbol << c.name << c.category << c.price << c.changePercent;

    check(!commodities.isEmpty(), "live catalog returned at least one commodity");

    bool allHaveIdentity = true;
    for (const Commodity &c : commodities) {
        if (c.name.isEmpty() || c.symbol.isEmpty() || c.category.isEmpty()) {
            allHaveIdentity = false;
            break;
        }
    }
    check(allHaveIdentity, "every commodity has a non-empty name/symbol/category");

    // --- SpreadFormulas -----------------------------------------------
    qInfo().noquote() << "\n=== Spread presets against live data ===";
    for (const SpreadPreset &preset : spreadPresets()) {
        const auto value = computeSpread(preset, commodities);
        qInfo().noquote() << " " << preset.id << "->" << (value ? QString::number(*value, 'f', 4) : QStringLiteral("no data"));
    }

    const bool hasGold = std::any_of(commodities.begin(), commodities.end(),
                                      [](const Commodity &c) { return c.name.toLower().contains(QStringLiteral("gold")); });
    const bool hasSilver = std::any_of(commodities.begin(), commodities.end(), [](const Commodity &c) {
        return c.name.toLower().contains(QStringLiteral("silver"));
    });
    if (hasGold && hasSilver) {
        const SpreadPreset *goldSilver = nullptr;
        for (const SpreadPreset &preset : spreadPresets()) {
            if (preset.id == QStringLiteral("gold-silver"))
                goldSilver = &preset;
        }
        const auto ratio = goldSilver ? computeSpread(*goldSilver, commodities) : std::nullopt;
        check(ratio.has_value(), "gold-silver spread computes when both legs are present in the live catalog");
        if (ratio)
            check(*ratio > 5.0 && *ratio < 500.0,
                  QStringLiteral("gold-silver ratio (%1) is in a plausible range").arg(*ratio, 0, 'f', 2));
    } else {
        qInfo().noquote() << "  (skipping gold-silver check: live catalog doesn't currently include both legs)";
    }

    // --- CommodityTableModel -------------------------------------------
    CommodityTableModel model;
    model.setCommodities(commodities);
    check(model.rowCount() == commodities.size(), "CommodityTableModel row count matches the live catalog size");

    bool headersOk = true;
    for (int col = 0; col < CommodityTableModel::ColumnCount; ++col) {
        if (model.headerData(col, Qt::Horizontal).toString().isEmpty())
            headersOk = false;
    }
    check(headersOk, "every CommodityTableModel column has a header label");

    // --- CommodityFilterProxyModel: category filter correctness --------
    CommodityFilterProxyModel proxy;
    proxy.setSourceModel(&model);

    QSet<QString> categories;
    for (const Commodity &c : commodities)
        categories.insert(c.category);

    bool categoryFilterOk = true;
    for (const QString &category : std::as_const(categories)) {
        proxy.setCategory(category);
        int expected = 0;
        for (const Commodity &c : commodities) {
            if (c.category == category)
                ++expected;
        }
        if (proxy.rowCount() != expected) {
            categoryFilterOk = false;
            break;
        }
        for (int r = 0; r < proxy.rowCount(); ++r) {
            // CommodityTableModel's Category column is display-formatted
            // (capitalized), so compare case-insensitively against the raw
            // Commodity::category value used as the filter key.
            const QModelIndex sourceIndex = proxy.mapToSource(proxy.index(r, CommodityTableModel::ColumnCategory));
            if (model.data(sourceIndex).toString().compare(category, Qt::CaseInsensitive) != 0) {
                categoryFilterOk = false;
                break;
            }
        }
    }
    check(categoryFilterOk, QStringLiteral("category filter row counts/contents are correct for all %1 live categories").arg(categories.size()));
    proxy.setCategory(QStringLiteral("all"));

    // --- CommodityFilterProxyModel: numeric (not lexicographic) sort ---
    // Real price magnitudes span orders of magnitude (e.g. $2.7 gas vs $4000+
    // gold) so, unlike small hand-picked fixtures, a lexicographic-string-sort
    // regression is essentially guaranteed to show up here.
    proxy.sort(CommodityTableModel::ColumnPrice, Qt::AscendingOrder);
    bool sortedAscending = true;
    double previousPrice = -1.0;
    for (int r = 0; r < proxy.rowCount(); ++r) {
        const double price = proxy.data(proxy.index(r, CommodityTableModel::ColumnPrice), Qt::EditRole).toDouble();
        if (price < previousPrice) {
            sortedAscending = false;
            break;
        }
        previousPrice = price;
    }
    check(sortedAscending, "sorting by Price is numeric (non-decreasing), not lexicographic");

    qInfo().noquote() << QStringLiteral("\n%1 check(s) failed.").arg(g_failures);
    return g_failures == 0 ? 0 : 1;
}
