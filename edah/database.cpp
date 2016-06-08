/*
    Edah
    Copyright (C) 2016  Lukasz Matczak

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "database.h"
#include <libedah/logger.h>

#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>

Database::Database(QObject *parent) : QObject(parent)
{
    const QString configLocation = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

    if(!QDir(configLocation).exists())
        QDir().mkdir(configLocation);


    {
        db = QSqlDatabase::addDatabase("QSQLITE");
        db.setDatabaseName(configLocation + "/config.db");
        if(!db.open())
        {
            LOG(QString("Couldn't open database \"%1\"").arg(db.databaseName()));
        }

        db.exec("CREATE TABLE IF NOT EXISTS config (module TEXT, key TEXT, value BLOB, PRIMARY KEY(module, key))");
    }

}

Database::~Database()
{
    db.close();
}

QVariant Database::getValue(const QString &key, const QVariant &defaultValue)
{
    QSqlQuery q(db);
    q.prepare("SELECT `value` FROM `config` WHERE `key`=:key AND `module`=:module");
    q.bindValue(":key", key);
    q.bindValue(":module", "core");
    q.exec();

    q.first();
    QVariant ret = q.value(0);

    return ret.isValid() ? ret : defaultValue;
}

void Database::setValue(const QString &key, const QVariant &value)
{
    QSqlQuery q(db);
    q.prepare("INSERT OR IGNORE INTO `config` VALUES(:module, :key, 0)");
    q.bindValue(":key", key);
    q.bindValue(":module", "core");
    q.exec();

    q.prepare("UPDATE `config` SET `value`=:value WHERE `key`=:key AND `module`=:module");
    q.bindValue(":value", value);
    q.bindValue(":key", key);
    q.bindValue(":module", "core");
    q.exec();
}
