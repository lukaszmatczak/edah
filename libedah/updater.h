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

#ifndef UPDATER_H
#define UPDATER_H

#include "libedah.h"

#include <QObject>

#include <QNetworkAccessManager>

struct UpdateInfo
{
    QString name;
    QString oldVersion;
    QString newVersion;
    int oldBuild;
    int newBuild;
};
typedef QVector<UpdateInfo> UpdateInfoArray;

class LIBEDAHSHARED_EXPORT Updater : public QObject
{
    Q_OBJECT
public:
    explicit Updater(QObject *parent = 0);
    void setInstallDir(QString dir);

public slots:
    void checkUpdates();
    void checkFiles();

private slots:
    void readyReadGet_build();
    void readyReadGet();

private:
    QJsonObject JsonFindModule(const QJsonArray &arr, const QJsonValue &name);
    QString decryptChecksum(QString encrypted);
    QStringList getInstalledPlugins();

    QString installDir;

    QStringList pluginsList;
    QSet<QString> depedencies;

    QNetworkAccessManager *manager;
    QNetworkReply *reply;

signals:
    void newUpdates(UpdateInfoArray info);
    void filesChecked(int size);
};

#endif // UPDATER_H
