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

LIBEDAHSHARED_EXPORT Database *settings;

/*!
 \class Database
 \brief The Database class provides access to sqlite database.

 Don't create and delete this object manually. Instance of this class is
 created automatically by core module as global object:
 \code
 Database *settings;
 \endcode

 Database file is stored in "%USERPROFILE%\AppData\Local\edah\config.db"
 on Windows and "~/.config/edah/config.db" on Linux.
 */

Database::Database()
{
    db = this->createDatabaseConnection("settings");

    db.exec("CREATE TABLE IF NOT EXISTS config (module TEXT, key TEXT, value BLOB, PRIMARY KEY(module, key))");
}

Database::~Database()
{
    db.commit();
    db.close();
}

/*!
 * \brief Creates database connection with given \a connectionName.
 *
 * You can create additional connection if you want to create and browse own
 * table in config database.
 *
 * \note \a connectionName must be unique in whole application, otherwise
 * this method will return an invalid QSqlDatabase object.
 *
 * \sa QSqlDatabase::isValid()
 */
QSqlDatabase Database::createDatabaseConnection(const QString &connectionName)
{
    if(QSqlDatabase::contains(connectionName))
    {
        LOG(QString("Connection named \"%1\" already exists!")
            .arg(connectionName));

        return QSqlDatabase();
    }

    const QString configLocation = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

    if(!QDir(configLocation).exists())
        QDir().mkdir(configLocation);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(configLocation + "/config.db");
    if(!db.open())
    {
        LOG(QString("Couldn't open database \"%1\"").arg(db.databaseName()));
    }

    return db;
}

/*!
 * \brief Returns the value for setting \a key and owner \a plugin.
 *
 * If the setting doesn't exist, returns \a defaultValue. If no default value
 * is specified, a default QVariant is returned.
 *
 * Example:
 * \code
 * QString defaultDir = settings->value(this, "defaultDir").toString();
 * \endcode
 * \sa Database::setValue()
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
 * \brief Sets the value of \a key to \a value and owner to \a plugin.
 *
 * If the \a key already exists, the previous value is overwritten.
 *
 * Example:
 * \code
 * settings->setValue(this, "defaultDir", "D:/records/");
 * \endcode
 * \sa Database::value()
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
