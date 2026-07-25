#pragma once

#include <QJsonObject>
#include <QString>

// Mirrors the `portfolios` table row. Free tier gets exactly one (the
// server auto-creates/assigns it on first position insert); premium/pro can
// have several.
struct Portfolio
{
    QString id;
    QString name;
    bool isDefault = false;

    static Portfolio fromJson(const QJsonObject &json);
};
