#include "OhlcBar.h"

OhlcBar OhlcBar::fromJson(const QJsonObject &json)
{
    OhlcBar bar;
    bar.date = json.value("date").toString();

    // Intraday ('1d') responses only carry {date, price} with no OHLC spread.
    const double price = json.value("price").toDouble();
    bar.open = json.contains("open") ? json.value("open").toDouble() : price;
    bar.high = json.contains("high") ? json.value("high").toDouble() : price;
    bar.low = json.contains("low") ? json.value("low").toDouble() : price;
    bar.close = json.contains("close") ? json.value("close").toDouble() : price;
    return bar;
}
