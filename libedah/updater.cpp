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

#include "updater.h"

#include <utils.h>
#include <logger.h>

#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

#include <QNetworkRequest>
#include <QNetworkReply>

#include <Windows.h>

Updater::Updater(QObject *parent) : QObject(parent)
{

}

void Updater::setInstallDir(QString dir)
{
    this->installDir = dir + "/";
    this->manager = nullptr;

    qRegisterMetaType<UpdateInfoArray>("UpdateInfoArray");
}

QJsonObject Updater::JsonFindModule(const QJsonArray &arr, const QJsonValue &name)
{
    for(int i=0; i<arr.size(); i++)
    {
        if(arr[i].toObject().value("name") == name)
            return arr[i].toObject();
    }

    return QJsonObject();
}

///////////////
/// STAGE 1 ///
///////////////

void Updater::checkUpdates()
{
    OSVERSIONINFO winver;
    ZeroMemory(&winver, sizeof(OSVERSIONINFO));
    winver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&winver);

    BYTE machineID[255];
    DWORD machineIDsize = 255;
    HKEY hKey;
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &hKey);
    RegQueryValueEx(hKey, L"MachineGuid", NULL, NULL, machineID, &machineIDsize);
    RegCloseKey(hKey);

    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(QByteArray((char*)machineID, machineIDsize));
    QString machineHash = hash.result().toHex().left(8);

    if(!manager) manager = new QNetworkAccessManager;
    QNetworkRequest url(QUrl(utils->getServerUrl() + "/api/get_build.php"));
    url.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    url.setRawHeader("User-Agent",
                     QString("Edah/%1 (Windows NT %2.%3.%4)")
                     .arg(/*currBuild*/0)
                     .arg(winver.dwMajorVersion)
                     .arg(winver.dwMinorVersion)
                     .arg(winver.dwBuildNumber).toUtf8());
    reply = manager->post(url, ("i="+machineHash).toUtf8());

    connect(reply, &QNetworkReply::finished, this, &Updater::readyReadGet_build);
}

void Updater::readyReadGet_build()
{
    UpdateInfoArray updates;

    QJsonArray remoteJson = QJsonDocument::fromJson(reply->readAll()).array();

    QFile file(this->installDir + "version.json");
    if(!file.open(QIODevice::ReadOnly))
    {
        LOG(QString("Couldn't open file \"%1\"!").arg(file.fileName()));
        return;
    }

    QJsonArray localJson = QJsonDocument::fromJson(file.readAll()).array();

    for(int i=0; i<localJson.size(); i++)
    {
        QJsonObject local = localJson[i].toObject();
        QJsonObject remote = JsonFindModule(remoteJson, local.value("name"));
        if(local.value("b").toInt() < remote.value("b").toInt())
        {
            UpdateInfo info;
            info.name = local.value("name").toString();
            info.oldVersion = local.value("v").toString();
            info.oldBuild = local.value("b").toInt();
            info.newVersion = remote.value("v").toString();
            info.newBuild = remote.value("b").toInt();

            updates.push_back(info);
        }
    }

    if(updates.size() > 0)
    {
        emit newUpdates(updates);
    }
}
