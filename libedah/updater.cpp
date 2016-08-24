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
#include <QDir>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

#include <QNetworkRequest>
#include <QNetworkReply>

#include <Windows.h>

#include <openssl/rsa.h>
#include <openssl/pem.h>

const char *publicKey = "-----BEGIN PUBLIC KEY-----\n"
        "MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAu/OBNKeZHdaAn1kx35fd\n"
        "HYR2ARHUuEWT9jW1R0oHIXy7htL5NYK6Vei4nHudRi6I/uw1EmmrYMT9S+ZKXwlW\n"
        "bMKESuXnQoSQZ92lb+YMoBmCS1whavJh1/6FiUD/PjF1taEGAkVBKChckhmhOXA6\n"
        "bCuT7E+kg8czhD+rHw0QAPWvV90o3ugiM5p7GmcXN842g3FleSDDWcHoOWsWv1/X\n"
        "9C1W8tp70QfThP2Bg7J7xKmPkH0+LSV477LOhkrPAfz1jQnOlu8A0U4zC+jx7H6d\n"
        "3RE4gPJlDrtyrgCtnx9IvxtXfH5bd1qrLqS15rblB8/rJvMKFLAi1vrdwMweAoml\n"
        "Rb6U5r6DnZXYVxks8LsWhree0KhE7/R+3rS+i4C4Df5haniAijzGU9+A7R46z4LV\n"
        "JEsAF3ANq/Gw/JzAVT4Dl43Ig5MDthx9ovqDbHWWxgYQQKX7pKIvabevQOccEH7R\n"
        "z+OqRm9aOyw9QFwmR4FyOJkRCpTk9nby3WCTeBuwMbwtb8Ead8fYoRtS80sJ4Jb5\n"
        "m6ekP3sYvEebybmL5B0Hz3HIxERD2nU7vArkCezZ0e06r4jcXvb78wcJ2tww+j6u\n"
        "9Hu5f+sqlgMFNADdJrUUadmOHOOTsH9HompudwJUw1HEvmBtGju4oMe0scN5DDDo\n"
        "FGh6dx/iDPPsdSZztySqgBsCAwEAAQ==\n"
        "-----END PUBLIC KEY-----\n";

Updater::Updater(QObject *parent) : QObject(parent)
{
    this->manager = nullptr;

    qRegisterMetaType<UpdateInfoArray>("UpdateInfoArray");
}

void Updater::setInstallDir(QString dir)
{
    this->installDir = dir + "/";
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

QString Updater::decryptChecksum(QString encrypted)
{
    QString ret;

    RSA *rsa = NULL;
    BIO *bio = BIO_new_mem_buf((void*)publicKey, -1);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL) ;
    rsa = PEM_read_bio_RSA_PUBKEY(bio, NULL, NULL, NULL);

    char *dec = new char[RSA_size(rsa)];

    int size = RSA_public_decrypt(QByteArray::fromBase64(encrypted.toLocal8Bit()).length(),
                       (unsigned char*)QByteArray::fromBase64(encrypted.toLocal8Bit()).data(),
                       (unsigned char*)dec,
                       rsa,
                       RSA_PKCS1_PADDING);
    if(size > -1)
    {
        ret = QByteArray((char*)dec, size).toHex();
    }
    else
    {
        LOG("Cannot decrypt checksum!");
    }

    RSA_free(rsa);
    BIO_free(bio);
    delete [] dec;

    return ret;
}

QStringList Updater::getInstalledPlugins()
{
    return QDir(utils->getDataDir()+"/plugins").entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
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

    depedencies.clear();
    depedencies.insert("core");

    pluginsList = getInstalledPlugins();
    for(int i=0; i<pluginsList.size(); i++)
    {
        QFile file(utils->getDataDir()+"/plugins/"+pluginsList[i]+"/info.json");
        file.open(QIODevice::ReadOnly);

        QJsonObject pluginJson = QJsonDocument::fromJson(file.readAll()).object();

        QJsonObject remote = JsonFindModule(remoteJson, pluginJson.value("id"));

        QStringList dep = remote.value("d").toString().split(" ", QString::SkipEmptyParts);
        for(int i=0; i<dep.size(); i++)
            depedencies.insert(dep[i]);

        if(pluginJson.value("build").toInt() < remote.value("b").toInt(0))
        {
            UpdateInfo info;
            info.name = pluginJson.value("id").toString();
            info.oldVersion = pluginJson.value("version").toString();
            info.oldBuild = pluginJson.value("build").toInt();
            info.newVersion = remote.value("v").toString();
            info.newBuild = remote.value("b").toInt();

            updates.push_back(info);
        }
    }

    QJsonArray localJson = QJsonDocument::fromJson(file.readAll()).array();

    for(int i=0; i<localJson.size(); i++)
    {
        QJsonObject local = localJson[i].toObject();

        if(!depedencies.contains(local.value("name").toString())) continue;

        QJsonObject remote = JsonFindModule(remoteJson, local.value("name"));
        if(local.value("b").toInt() < remote.value("b").toInt(0))
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

///////////////
/// STAGE 2 ///
///////////////

void Updater::checkFiles()
{
    if(!manager) manager = new QNetworkAccessManager;
    QNetworkRequest url(QUrl(utils->getServerUrl() + "/api/get.php"));
    reply = manager->get(url);
    connect(reply, &QNetworkReply::finished, this, &Updater::readyReadGet);
}

void Updater::readyReadGet()
{
    QList<QByteArray> data = reply->readAll().split('\n');

    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(data[1]);
    QString checksum = hash.result().toHex();

    if(decryptChecksum(data[0]) != checksum)
    {
        LOG("Checksum mismatch!");
        emit filesChecked(0);

        return;
    }

    QJsonObject modules = QJsonDocument::fromJson(data[1]).object();
    int updateSize = 0;

    for(int i=0; i<modules.size(); i++)
    {
        QString name = modules.keys()[i];

        if(pluginsList.contains(name) || modules.contains(name))
        {
            QJsonArray files = modules.value(name).toArray();
            QString prefix = "";
            if(pluginsList.contains(name)) prefix = "plugins/"+name+"/";

            for(int i=0; i<files.size(); i++)
            {
                QString filename = files[i].toObject().value("n").toString();
                QString checksum = QByteArray::fromBase64(files[i].toObject().value("c").toString().toUtf8()).toHex();
                int size = files[i].toObject().value("cs").toInt();

                QFile file(installDir + prefix + filename);

                if(!file.open(QIODevice::ReadOnly))
                {
                    updateSize += size;
                    //filesToUpdate << filename;
                    continue;
                }
                QCryptographicHash hash(QCryptographicHash::Sha256);
                hash.addData(&file);
                QString localChecksum = hash.result().toHex();
                file.close();

                if(checksum != localChecksum)
                {
                    updateSize += size;
                    //filesToUpdate << filename;
                }
            }
        }
    }

    emit filesChecked(updateSize);
}
