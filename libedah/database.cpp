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
#include "logger.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>

LIBEDAHSHARED_EXPORT Database *db;

/*!
 \class Database
 \inmodule Edah
 \brief The Database class provides access to sqlite database.

 Instance of this class is created automatically by core module as global object:
 \code
 Database *db;
 \endcode

 Database file is stored in " %USERPROFILE%\\AppData\\Local\\edah\\config.db"
 on Windows and "~/.config/edah/config.db" on Linux.
 */

/*!
 \variable Database::db
 \brief The QSqlDatabase object. You can use it if you want to create and browse
 own table in config database.
 */

/*!
 \fn Database::Database(QObject *parent)
 Don't create this object manually. Core module creates it automatically.
 */
Database::Database(QObject *parent) : QObject(parent)
{
    const QString configLocation = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

    if(!QDir(configLocation).exists())
        QDir().mkdir(configLocation);

    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(configLocation + "/config.db");
    if(!db.open())
    {
        LOG(QString("Couldn't open database \"%1\"").arg(db.databaseName()));
    }

    db.exec("CREATE TABLE IF NOT EXISTS config (module TEXT, key TEXT, value BLOB, PRIMARY KEY(module, key))");
}

/*!
 \fn Database::~Database()
 Don't delete this object manually. Core module deletes it automatically.
 */
Database::~Database()
{
    db.close();
}

/*!
 \fn QVariant Database::value(const IPlugin *plugin, const QString &key, const QVariant &defaultValue)
 Returns the value for setting \a key and owner \a plugin. If the setting
 doesn't exist, returns \a defaultValue.

 If no default value is specified, a default QVariant is returned.

 Example:
 \code
 QString defaultDir = db->value(this, "defaultDir").toString();
 \endcode
 */
QVariant Database::value(const IPlugin *plugin, const QString &key, const QVariant &defaultValue)
{
    QString module = "core";
    if(plugin)
    {
        module = plugin->getPluginId();
    }

    QSqlQuery q(db);
    q.prepare("SELECT `value` FROM `config` WHERE `key`=:key AND `module`=:module");
    q.bindValue(":key", key);
    q.bindValue(":module", module);
    q.exec();

    q.first();
    QVariant ret = q.value(0);

    return ret.isValid() ? ret : defaultValue;
}

/*!
 \fn void Database::setValue(const IPlugin *plugin, const QString &key, const QVariant &value)
 Sets the value of \a key to \a value and owner to \a plugin. If the \a key
 already exists, the previous value is overwritten.

 Example:
 \code
 db->setValue(this, "defaultDir", "D:/records/");
 \endcode
 */
void Database::setValue(const IPlugin *plugin, const QString &key, const QVariant &value)
{
    QString module = "core";
    if(plugin)
    {
        module = plugin->getPluginId();
    }

    QSqlQuery q(db);
    q.prepare("INSERT OR IGNORE INTO `config` VALUES(:module, :key, 0)");
    q.bindValue(":key", key);
    q.bindValue(":module", module);
    q.exec();

    q.prepare("UPDATE `config` SET `value`=:value WHERE `key`=:key AND `module`=:module");
    q.bindValue(":value", value);
    q.bindValue(":key", key);
    q.bindValue(":module", module);
    q.exec();
}
