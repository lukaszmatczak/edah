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
#include <QJsonObject>
#include <QJsonArray>

#include <QNetworkAccessManager>

struct UpdateInfo
{
    enum Action { Update, Install, Uninstall };

    Action action;
    QString name;
    QString oldVersion;
    QString newVersion;
    int oldBuild;
    int newBuild;
};
typedef QVector<UpdateInfo> UpdateInfoArray;

struct FileInfo
{
    enum Type { Directory, File };

    Type type;
    bool isPlugin;
    QString module;
    QString filename;
    QString checksum;
    QString compressedChecksum;
    quint32 size;
    int compressedSize;
};

struct UpdateInfoEx
{
    UpdateInfoArray updates;
    QJsonArray remoteJson;
    QSet<QString> depedencies;
    QJsonObject modules;
    QList<FileInfo> filesToUpdate;
    int totalDownloadSize;
};

class LIBEDAHSHARED_EXPORT Updater : public QObject
{
    Q_OBJECT
public:
    explicit Updater(QObject *parent = 0);
    void setInstallDir(QString dir);
    void setInstallPlugin(QString plugin);

public slots:
    UpdateInfoEx checkUpdates(); // stage 1
    UpdateInfoEx checkFiles(); // stage 2
    void prepareUpdate(); // stage 3
    void doUpdate(); // stage 4
    void uninstallPlugin();

private:
    QStringList getInstalledPlugins();

    // stage 1
    QJsonArray download_getBuild();
    void checkForPluginsUpdate(const QJsonArray &remoteJson, QSet<QString> *depedencies, UpdateInfoArray *updates);
    void checkForModulesUpdate(const QJsonArray &remoteJson, QSet<QString> depedencies, UpdateInfoArray *updates);
    QJsonObject JsonFindModule(const QJsonArray &arr, const QJsonValue &name);

    // stage 2
    QByteArray download_get();
    QString decryptChecksum(QString encrypted);
    QList<FileInfo> compareChecksums(const QJsonObject &modules, const QSet<QString> &depedencies);

    // stage 4
    void downloadUpdates(const QList<FileInfo> &filesToUpdate, int filesSize);
    bool verify(const QList<FileInfo> &filesToUpdate);
    void installUpdate(const QList<FileInfo> &filesToUpdate);
    void cleanupDepedencies(const QSet<QString> &depedencies);
    void runPostinstScripts(const UpdateInfoArray &updates);
    void updateVersionInfo(const QSet<QString> &depedencies, const QJsonArray &versions, const QJsonObject &modules);

    QString installDir;
    QString updateDir;
    QString installPlugin;

    QNetworkAccessManager *manager;
    QNetworkReply *reply;

signals:
    void newUpdates(UpdateInfoArray info);
    void filesChecked(int size);
    void progress(int stage, int curr, int total);
    void verFailed();
    void updateFinished();
};

#endif // UPDATER_H
