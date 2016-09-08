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
#include <QCoreApplication>
#include <QJsonDocument>
#include <QDebug>
#include <QDataStream>

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

    updateDir = QDir::tempPath() + "/edah";

    qRegisterMetaType<UpdateInfoArray>("UpdateInfoArray");
}

void Updater::setInstallDir(QString dir)
{
    this->installDir = dir + "/";
}

QStringList Updater::getInstalledPlugins()
{
    return QDir(this->installDir+"/plugins").entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
}

///////////////
/// STAGE 1 ///
///////////////

QPair<QJsonArray, QSet<QString> > Updater::checkUpdates()
{
    UpdateInfoArray updates;
    QJsonArray remoteJson = this->download_getBuild();

    QSet<QString> depedencies;
    depedencies.insert("core");

    this->checkForPluginsUpdate(remoteJson, &depedencies, &updates);
    this->checkForModulesUpdate(remoteJson, depedencies, &updates);

    if(updates.size() > 0)
    {
        emit newUpdates(updates);
    }

    return qMakePair(remoteJson, depedencies);
}

QJsonArray Updater::download_getBuild()
{
    OSVERSIONINFO winver;
    ZeroMemory(&winver, sizeof(OSVERSIONINFO));
    winver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&winver);

    if(!manager) manager = new QNetworkAccessManager;
    QNetworkRequest url(QUrl(utils->getServerUrl() + "/api/get_build.php"));
    url.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    url.setRawHeader("User-Agent",
                     QString("Edah/%1 (Windows NT %2.%3.%4)")
                     .arg(/*currBuild*/0)
                     .arg(winver.dwMajorVersion)
                     .arg(winver.dwMinorVersion)
                     .arg(winver.dwBuildNumber).toUtf8());
    reply = manager->post(url, ("i="+this->getDeviceId()).toUtf8());

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    reply->deleteLater();

    return QJsonDocument::fromJson(reply->readAll()).array();
}

void Updater::checkForPluginsUpdate(const QJsonArray &remoteJson, QSet<QString> *depedencies, UpdateInfoArray *updates)
{
    QStringList pluginsList = getInstalledPlugins();
    for(int i=0; i<pluginsList.size(); i++)
    {
        QFile file(this->installDir+"/plugins/"+pluginsList[i]+"/info.json");
        if(!file.open(QIODevice::ReadOnly))
        {
            LOG(QString("Couldn't open file \"%1\"!").arg(file.fileName()));
            continue;
        }

        QJsonObject pluginJson = QJsonDocument::fromJson(file.readAll()).object();

        QJsonObject remote = JsonFindModule(remoteJson, pluginJson.value("id"));

        QStringList dep = remote.value("d").toString().split(" ", QString::SkipEmptyParts);
        for(int i=0; i<dep.size(); i++)
            depedencies->insert(dep[i]);

        if(pluginJson.value("build").toInt() < remote.value("b").toInt(0))
        {
            UpdateInfo info;
            info.name = pluginJson.value("id").toString();
            info.oldVersion = pluginJson.value("version").toString();
            info.oldBuild = pluginJson.value("build").toInt();
            info.newVersion = remote.value("v").toString();
            info.newBuild = remote.value("b").toInt();

            updates->push_back(info);
        }
    }
}

void Updater::checkForModulesUpdate(const QJsonArray &remoteJson, QSet<QString> depedencies, UpdateInfoArray *updates)
{
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

        if(!depedencies.contains(local.value("name").toString()))
            continue;

        QJsonObject remote = JsonFindModule(remoteJson, local.value("name"));
        if(local.value("b").toInt() < remote.value("b").toInt(0))
        {
            UpdateInfo info;
            info.name = local.value("name").toString();
            info.oldVersion = local.value("v").toString();
            info.oldBuild = local.value("b").toInt();
            info.newVersion = remote.value("v").toString();
            info.newBuild = remote.value("b").toInt();

            updates->push_back(info);
        }
    }
}

QString Updater::getDeviceId()
{
    BYTE machineID[255];
    DWORD machineIDsize = 255;
    HKEY hKey;
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &hKey);
    RegQueryValueEx(hKey, L"MachineGuid", NULL, NULL, machineID, &machineIDsize);
    RegCloseKey(hKey);

    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(QByteArray((char*)machineID, machineIDsize));
    return hash.result().toHex().left(8);
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
/// STAGE 2 ///
///////////////

UpdateInfoEx Updater::checkFiles()
{
    UpdateInfoEx info;

    QList<QByteArray> data = this->download_get().split('\n');

    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(data[1]);
    QString checksum = hash.result().toHex();

    if(decryptChecksum(data[0]) != checksum)
    {
        LOG("Checksum mismatch!");
        emit filesChecked(0);

        return info;
    }

    QPair<QJsonArray, QSet<QString> > pair = this->checkUpdates();

    info.remoteJson = pair.first;
    info.depedencies = pair.second;
    info.modules = QJsonDocument::fromJson(data[1]).object();
    info.filesToUpdate = this->compareChecksums(info.modules, info.depedencies);
    info.totalDownloadSize = 0;

    for(int i=0; i<info.filesToUpdate.size(); i++)
    {
        info.totalDownloadSize += info.filesToUpdate[i].compressedSize;
    }

    emit filesChecked(info.totalDownloadSize);

    return info;
}

QByteArray Updater::download_get()
{
    if(!manager) manager = new QNetworkAccessManager;
    QNetworkRequest url(QUrl(utils->getServerUrl() + "/api/get.php"));
    reply = manager->get(url);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    reply->deleteLater();

    return reply->readAll();
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

QList<FileInfo> Updater::compareChecksums(const QJsonObject &modules, const QSet<QString> &depedencies)
{
    QList<FileInfo> filesToUpdate;
    QStringList pluginsList = this->getInstalledPlugins();

    for(int i=0; i<modules.size(); i++)
    {
        QString name = modules.keys()[i];

        if(pluginsList.contains(name) || depedencies.contains(name))
        {
            QJsonArray files = modules.value(name).toArray();

            for(int i=0; i<files.size(); i++)
            {
                FileInfo fi;
                fi.type = files[i].toObject()["t"].toString() == "f" ? FileInfo::File : FileInfo::Directory;
                fi.isPlugin = files[i].toObject().value("p").toInt();
                fi.module = name;
                fi.filename = files[i].toObject().value("n").toString();
                fi.checksum = QByteArray::fromBase64(files[i].toObject().value("c").toString().toUtf8()).toHex();
                fi.compressedChecksum = QByteArray::fromBase64(files[i].toObject().value("cc").toString().toUtf8()).toHex();
                fi.size = files[i].toObject().value("s").toInt();
                fi.compressedSize = files[i].toObject().value("cs").toInt();

                QString localChecksum;
                QString prefix = fi.isPlugin ? "plugins/"+name+"/" : "";
                QFile file(this->installDir + prefix + fi.filename);

                if(file.open(QIODevice::ReadOnly))
                {
                    QCryptographicHash hash(QCryptographicHash::Sha256);
                    hash.addData(&file);
                    localChecksum = hash.result().toHex();
                }

                if(fi.checksum != localChecksum)
                {
                    filesToUpdate << fi;
                }
            }
        }
    }

    return filesToUpdate;
}

///////////////
/// STAGE 3 ///
///////////////

void Updater::prepareUpdate()
{
    QStringList dirs;
    dirs << "" << "platforms";

    for(int i=0; i<dirs.size(); i++)
    {
        QDir().mkpath(updateDir + "/" + dirs[i]);
    }

    QStringList files;
    files << "Qt5Core.dll" << "Qt5Gui.dll" << "Qt5Widgets.dll" << "Qt5Network.dll" << "platforms/qwindows.dll" << "updater.exe" << "edah.dll" << "libeay32MD.dll";
    //files << "Qt5Cored.dll" << "Qt5Guid.dll" << "Qt5Widgetsd.dll" << "Qt5Networkd.dll" << "platforms/qwindows.dll" << "updater.exe" << "edah.dll" << "libeay32MD.dll";; // debug
    for(int i=0; i<files.size(); i++)
    {
        QFile::copy(installDir + files[i], updateDir + "/" + files[i]);
    }

    ShellExecuteW(0,
                  L"runas",
                  (LPCWSTR)(updateDir + "/updater.exe").utf16(),
                  (LPCWSTR)("\"" + installDir + "\"").utf16(),
                  (LPCWSTR)updateDir.utf16(),
                  SW_SHOWNORMAL);

    qApp->quit();
}

///////////////
/// STAGE 4 ///
///////////////

void Updater::doUpdate()
{
    UpdateInfoEx info = this->checkFiles();
    this->downloadUpdates(info.filesToUpdate, info.totalDownloadSize);
    this->verify(info.filesToUpdate);
    this->installUpdate(info.filesToUpdate, info.depedencies, info.remoteJson);

    QDir(updateDir).removeRecursively();
    emit updateFinished();
}

void Updater::downloadUpdates(const QList<FileInfo> &filesToUpdate, int filesSize)
{
    QDir().mkdir(updateDir + "/compressed");

    int bytesDownloaded = 0;

    for(int i=0; i<filesToUpdate.size(); i++)
    {
        if(filesToUpdate[i].type == FileInfo::File)
        {
            QFile currFile(updateDir + "/compressed/" + filesToUpdate[i].checksum);
            currFile.open(QIODevice::WriteOnly);

            if(!manager) manager = new QNetworkAccessManager;
            QNetworkRequest url(QUrl(utils->getServerUrl() + "/api/compressed/" + filesToUpdate[i].checksum));
            reply = manager->get(url);

            connect(reply, &QNetworkReply::readyRead, this, [this, &currFile]() {
                currFile.write(reply->readAll());
            });
            connect(reply, &QNetworkReply::downloadProgress, this, [this, filesSize, bytesDownloaded](qint64 bytesReceived, qint64 bytesTotal) {
                Q_UNUSED(bytesTotal)
                emit progress(0, bytesDownloaded+bytesReceived, filesSize);
            });

            QEventLoop loop;
            connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();

            bytesDownloaded += filesToUpdate[i].compressedSize;
        }
    }
}

void Updater::verify(const QList<FileInfo> &filesToUpdate)
{
    for(int i=0; i<filesToUpdate.size(); i++)
    {
        emit progress(1, i+1, filesToUpdate.size());

        if(filesToUpdate[i].type == FileInfo::Directory)
            continue;

        QFile file(updateDir + "/compressed/" + filesToUpdate[i].checksum);

        if(!file.open(QIODevice::ReadOnly))
        {
            emit verFailed();
            return;
        }

        QCryptographicHash hash(QCryptographicHash::Sha256);
        hash.addData(&file);
        QString localChecksum = hash.result().toHex();

        if(filesToUpdate[i].compressedChecksum != localChecksum)
        {
            emit verFailed();
            return;
        }
    }
}

void Updater::installUpdate(const QList<FileInfo> &filesToUpdate, const QSet<QString> &depedencies, const QJsonArray &versions)
{
    for(int i=0; i<filesToUpdate.size(); i++)
    {
        emit progress(2, i+1, filesToUpdate.size());

        QString path = filesToUpdate[i].isPlugin ?
                    installDir + "/plugins/" + filesToUpdate[i].module + "/" + filesToUpdate[i].filename :
                    installDir + filesToUpdate[i].filename;

        if(filesToUpdate[i].type == FileInfo::Directory)
        {
            QDir().mkpath(path);
        }
        else
        {
            QDir().mkpath(QFileInfo(path).absoluteDir().path());

            if(QFile::exists(path))
            {
                QFile::remove(path);
            }

            QFile compressedFile(updateDir + "/compressed/" + filesToUpdate[i].checksum);
            compressedFile.open(QIODevice::ReadOnly);
            QByteArray compressed;
            QDataStream ds(&compressed, QIODevice::WriteOnly);
            ds.setByteOrder(QDataStream::BigEndian);
            ds << filesToUpdate[i].size;
            compressed += compressedFile.readAll();

            QFile destFile(path);
            destFile.open(QIODevice::WriteOnly);
            destFile.write(qUncompress(compressed));
        }
    }

    // update version JSON files
    QString json = "[";
    for(int i=0; i<versions.size(); i++)
    {
        QString name = versions[i].toObject()["name"].toString();
        if(depedencies.contains(name))
        {
            QString version = versions[i].toObject()["v"].toString();
            int build = versions[i].toObject()["b"].toInt();

            if(json.size() > 1)
                json += ",";

            json += QString("{\"name\":\"%1\",\"v\":\"%2\",\"b\":%3}")
                    .arg(name)
                    .arg(version)
                    .arg(build);
        }
    }
    json += "]";

    QFile versionJson(this->installDir + "version.json");
    if(!versionJson.open(QIODevice::WriteOnly))
    {
        LOG(QString("Couldn't open file \"%1\"!").arg(versionJson.fileName()));
        return;
    }

    versionJson.write(json.toUtf8());

    // TODO: depedencies
    // TODO: remove files
    // TODO: execute post-update scripts
}
