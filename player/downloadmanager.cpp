/*
    Edah
    Copyright (C) 2016-2017  Lukasz Matczak

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

#include "downloadmanager.h"
#include "player.h"

#include <libedah/logger.h>

#include <QBuffer>
#include <QDataStream>
#include <QDir>
#include <QMimeType>
#include <QMimeDatabase>

#include <QEventLoop>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>

#include <QSqlQuery>

#include <quazip/quazip.h>
#include <quazip/quazipfile.h>

const QString dbGetUrl = "https://apps.jw.org/GETPUBMEDIALINKS?output=json&alllangs=0";

const quint32 magic = 0xEDA0A0ED;
const quint32 currVersion = 2;

DownloadManager::DownloadManager(QString path, QString videoQuality, bool signLang) :
    path(path), videoQuality(videoQuality), QObject(nullptr), manager(nullptr)
{
    lang = tr("E");
    fileLang = signLang ? tr("BSL") : lang;
}

void DownloadManager::start()
{
    const QString issueFmt = "yyyyMM";
    const QDate currDate = QDate::currentDate();
    const QDate monDate = currDate.addDays(-currDate.dayOfWeek()+1);

    emit setTrayText(tr("Downloading"));

    this->db = QSqlDatabase::addDatabase("QSQLITE");

    QDir().mkpath(this->path);

    this->removeOldFiles();

    Playlist playlist;

    this->loadPlaylist(&playlist);

    int prevPlaylistSize = playlist.multimediaInfo[monDate.toString("yyyyMMdd")].size();
    if(prevPlaylistSize > 0)
        emit playlistLoaded(playlist.multimediaInfo[monDate.toString("yyyyMMdd")]);

    QThread::sleep(3);

    this->downloadAndParseProgram(&playlist, "mwb", monDate.toString(issueFmt));
    this->downloadAndParseProgram(&playlist, "w", monDate.addMonths(-2).toString(issueFmt));

    this->downloadAndParseProgram(&playlist, "mwb", monDate.addMonths(1).toString(issueFmt));
    this->downloadAndParseProgram(&playlist, "w", monDate.addMonths(-1).toString(issueFmt));

    this->getRemoteMultimediaInfo(&playlist.multimediaInfo);

    if(prevPlaylistSize == 0)
        emit playlistLoaded(playlist.multimediaInfo[monDate.toString("yyyyMMdd")]);

    this->savePlaylist(playlist);

    int downloadQueueBytes = 0;
    QStringList urlsToDownload = this->checkFilesToDownload(playlist.multimediaInfo, &downloadQueueBytes);
    this->downloadFiles(playlist.multimediaInfo, &urlsToDownload, &downloadQueueBytes);

    emit setTrayText("");
}

void DownloadManager::removeOldFiles()
{
    QDir dir(this->path);
    QStringList dirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    for(int i=0; i<dirs.size(); i++)
    {
        QDate dirDate = QDate::fromString(dirs[i], "yyyyMMdd");

        if(dirDate.isValid() && dirDate < QDate::currentDate().addDays(-QDate::currentDate().dayOfWeek()+1))
        {
            QDir(this->path + "/" + dirs[i]).removeRecursively();
        }
    }
}

RemoteInfo DownloadManager::getRemoteInfo(const QString &pub, const QString &issue)
{
    if(!manager) manager = new QNetworkAccessManager;
    QNetworkRequest url(QUrl(dbGetUrl + QString("&langwritten=%1&txtCMSLang=%1&issue=%2&pub=%3&fileformat=JWPUB")
                             .arg(lang)
                             .arg(issue)
                             .arg(pub)));
    url.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    reply = manager->get(url);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    reply->deleteLater();

    RemoteInfo info;

    if(reply->error() == QNetworkReply::NoError)
    {
        QJsonObject entry = QJsonDocument::fromJson(reply->readAll()).object()
                .value("files").toObject()
                .value(lang).toObject()
                .value("JWPUB").toArray()
                .at(0).toObject();

        info.url = entry.value("file").toObject()
                .value("url").toString();
        info.checksum = entry.value("file").toObject()
                .value("checksum").toString();
        info.size = entry.value("filesize").toInt();
    }

    return info;
}

RemoteInfo DownloadManager::getRemoteInfo(const MultimediaInfo &minfo)
{
    if(!manager) manager = new QNetworkAccessManager;
    QNetworkRequest url(QUrl(dbGetUrl + QString("&langwritten=%1&txtCMSLang=%1&issue=%2&pub=%3&docid=%4")
                             .arg(fileLang)
                             .arg(minfo.IssueTagNumber)
                             .arg(minfo.KeySymbol)
                             .arg(minfo.MepsDocumentId)));
    url.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    reply = manager->get(url);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    reply->deleteLater();

    RemoteInfo info;

    if(reply->error() == QNetworkReply::NoError)
    {
        QJsonObject fileTypes = QJsonDocument::fromJson(reply->readAll()).object()
                .value("files").toObject()
                .value(fileLang).toObject();

        for(auto it=fileTypes.begin(); it!=fileTypes.end(); ++it)
        {
            QJsonArray files = it->toArray();

            for(int i=0; i<files.size(); i++)
            {
                QJsonObject file = files[i].toObject();

                if(!minfo.Track.isEmpty() && minfo.Track.toInt() != file["track"].toInt())
                    continue;

                QMimeDatabase mimeDb;
                if(!minfo.MimeType.isEmpty() && mimeDb.mimeTypeForName(minfo.MimeType) != mimeDb.mimeTypeForName(file["mimetype"].toString()))
                    continue;

                QString label = file["label"].toString();
                if(label != "0p" && label != this->videoQuality)
                    continue;

                info.url = file["file"].toObject()["url"].toString();
                info.title = file["title"].toString();
                info.checksum = file["file"].toObject()["checksum"].toString();
                info.size = file["filesize"].toInt();

                return info;
            }
        }
    }

    return info;
}

void DownloadManager::getRemoteMultimediaInfo(QMap<QString, QList<MultimediaInfo> > *map)
{
    QStringList songs = Player::getSongSymbols();

    for(auto it=map->begin(); it!=map->end(); ++it)
    {
        for(int i=0; i<it->size(); i++)
        {
            if(!songs.contains(it->at(i).KeySymbol))
            {
                RemoteInfo info = this->getRemoteInfo(it->at(i));

                if(info.url.isEmpty())
                {
                    // TODO: err
                }
                else
                {
                    (*it)[i].url = info.url;
                    (*it)[i].title = info.title;
                    (*it)[i].size = info.size;
                    (*it)[i].checksum = info.checksum;
                }
            }
        }
    }
}

QStringList DownloadManager::checkFilesToDownload(const QMap<QString, QList<MultimediaInfo> > &map, int *downloadQueueBytes)
{
    QStringList urlsToDownload;
    for(auto it=map.keyBegin(); it!=map.keyEnd(); ++it)
    {
        for(int i=0; i<map[*it].size(); i++)
        {
            if(!map[*it][i].url.isEmpty())
            {
                QFile file(this->path + "/" + *it + "/" + QUrl(map[*it][i].url).fileName());
                QCryptographicHash hash(QCryptographicHash::Md5);

                if(file.open(QIODevice::ReadOnly))
                {
                    while(!file.atEnd())
                    {
                        hash.addData(file.read(1024*1024)); // 1MB chunks
                    }
                }

                if((map[*it][i].checksum.size() > 0) && (map[*it][i].checksum.compare(hash.result().toHex(), Qt::CaseInsensitive)) ||
                        map[*it][i].size != file.size())
                {
                    urlsToDownload << map[*it][i].url;
                    *downloadQueueBytes += map[*it][i].size;

                    this->sendStatus(*it, map[*it][i], false);
                }
                else
                {
                    this->sendStatus(*it, map[*it][i], true);
                }
            }
            else if(!Player::getSongSymbols().contains(map[*it][i].KeySymbol))
            {
                this->sendStatus(*it, map[*it][i], false);
            }
        }
    }

    return urlsToDownload;
}

void DownloadManager::downloadFiles(QMap<QString, QList<MultimediaInfo> > &map, QStringList *urlsToDownload, int *downloadQueueBytes)
{
    for(auto it=map.keyBegin(); it!=map.keyEnd(); ++it)
    {
        for(int i=0; i<map[*it].size(); i++)
        {
            emit setTrayText(tr("Downloading") +
                             tr("\n%n file(s) (%1 MB) left", "", urlsToDownload->size()).arg(*downloadQueueBytes/(1024*1024)));

            if(!map[*it][i].url.isEmpty() && urlsToDownload->contains(map[*it][i].url))
            {
                bool ok = this->downloadFile(&map[*it][i], this->path + "/" + *it + "/" + QUrl(map[*it][i].url).fileName());
                urlsToDownload->removeAll(map[*it][i].url);
                *downloadQueueBytes -= map[*it][i].size;

                this->sendStatus(*it, map[*it][i], ok);
            }
        }
    }
}

bool DownloadManager::downloadRemote(const RemoteInfo &info, QByteArray *dest)
{
    if(!manager) manager = new QNetworkAccessManager;
    QNetworkRequest request(QUrl{info.url});
    reply = manager->get(request);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    reply->deleteLater();

    if(reply->error() != QNetworkReply::NoError)
    {
        return false;
    }

    dest->swap(reply->readAll());

    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(*dest);
    QString localChecksum = hash.result().toHex();

    if(info.checksum.compare(localChecksum, Qt::CaseInsensitive))
    {
        return false;
    }

    return true;
}

void DownloadManager::downloadAndParseProgram(Playlist *playlist, const QString &pub, const QString &issue)
{
    RemoteInfo remoteInfo = this->getRemoteInfo(pub, issue);

    if(remoteInfo.url.isEmpty())
        return;

    ProgramInfo info(pub, issue, remoteInfo.checksum);

    if(!playlist->programInfo.contains(info))
    {
        QByteArray result;
        if(!this->downloadRemote(remoteInfo, &result))
            return;

        QBuffer resultBuf(&result);
        QuaZip zip(&resultBuf);
        if(zip.open(QuaZip::Mode::mdUnzip))
        {
            zip.setCurrentFile("contents");

            QuaZipFile contents(&zip);
            if(contents.open(QIODevice::ReadOnly))
            {
                QFile contentsFile(this->path + "/contents");
                if(contentsFile.open(QIODevice::WriteOnly))
                {
                    contentsFile.write(contents.readAll());
                    contentsFile.close();
                }
                contents.close();
            }
            zip.close();
        }

        QString dbName = QUrl(remoteInfo.url).fileName().replace(".jwpub", ".db");
        if(!this->extractFile(this->path + "/contents", dbName, this->path + "/" + dbName))
            return;

        QFile(this->path + "/contents").remove();


        db.setDatabaseName(this->path + "/" + dbName);
        if(!db.open())
        {
            LOG(QString("Couldn't open database \"%1\"").arg(db.databaseName()));
            return;
        }

        QSqlQuery q(db);
        q.prepare("SELECT `Multimedia`.`KeySymbol`, `Multimedia`.`Track`,`Multimedia`.`IssueTagNumber`, `Multimedia`.`MepsDocumentId`, `DatedText`.`FirstDateOffset`, `Multimedia`.`MimeType` "
                  "FROM `DocumentMultimedia`, `DatedText`, `Multimedia` "
                  "WHERE `DatedText`.`DocumentId`=`DocumentMultimedia`.`DocumentId`"
                  " AND `Multimedia`.`MultimediaId`=`DocumentMultimedia`.`MultimediaId`"
                  " AND `DocumentMultimedia`.`BeginParagraphOrdinal` BETWEEN `DatedText`.`BeginParagraphOrdinal` AND `DatedText`.`EndParagraphOrdinal`"); // TODO: ORDER BY
        q.exec();
        q.first();

        do
        {
            QString FirstDateOffset = q.value("FirstDateOffset").toString();

            QMutableListIterator<MultimediaInfo> i(playlist->multimediaInfo[FirstDateOffset]);
            while(i.hasNext())
            {
                MultimediaInfo info = i.next();
                if(info.weekend == (pub == "w"))
                {
                    i.remove();
                }
            }
        } while(q.next());

        q.first();

        do
        {
            MultimediaInfo info;

            QString FirstDateOffset = q.value("FirstDateOffset").toString();

            if(QDate::fromString(FirstDateOffset, "yyyyMMdd") < QDate::currentDate().addDays(-QDate::currentDate().dayOfWeek()+1))
                continue;

            info.weekend = (pub == "w");
            info.KeySymbol = q.value("KeySymbol").toString();
            info.Track = q.value("Track").toString();
            info.IssueTagNumber = q.value("IssueTagNumber").toString();
            info.MepsDocumentId = q.value("MepsDocumentId").toString();
            info.MimeType = q.value("MimeType").toString();

            playlist->multimediaInfo[FirstDateOffset].append(info);
        } while(q.next());

        db.close();

        QFile(this->path + "/" + dbName).remove();

        for(int i=0; i<playlist->programInfo.size(); i++)
        {
            if(playlist->programInfo[i].pub == info.pub && playlist->programInfo[i].issue == info.issue)
            {
                playlist->programInfo.remove(i);
                break;
            }
        }
        playlist->programInfo.append(info);
    }
}

bool DownloadManager::downloadFile(MultimediaInfo *info, const QString &local)
{
    RemoteInfo rinfo = this->getRemoteInfo(*info);
    info->url = rinfo.url;
    info->title = rinfo.url;
    info->size = rinfo.size;
    info->checksum = rinfo.checksum;

    if(!manager) manager = new QNetworkAccessManager;
    QNetworkRequest request(QUrl{info->url});
    reply = manager->get(request);

    QFile file(local + ".part");
    file.open(QIODevice::WriteOnly);

    QCryptographicHash hash(QCryptographicHash::Md5);

    connect(reply, &QNetworkReply::readyRead, this, [this, &file, &hash]() {
        QByteArray data = reply->readAll();
        file.write(data);
        hash.addData(data);
    });

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    reply->deleteLater();

    if(reply->error() != QNetworkReply::NoError)
    {
        return false;
    }

    if((info->checksum.size() > 0) && (info->checksum.compare(hash.result().toHex(), Qt::CaseInsensitive)) ||
            info->size != file.size())
    {
        return false;
    }

    QFile::remove(local);
    file.rename(local);

    return true;
}

bool DownloadManager::extractFile(QString zipFile, QString srcName, QString destName)
{
    bool ok = false;

    QuaZip zip;
    zip.setZipName(zipFile);
    if(zip.open(QuaZip::Mode::mdUnzip))
    {
        zip.setCurrentFile(srcName);
        QuaZipFile contents(&zip);
        if(contents.open(QIODevice::ReadOnly))
        {
            QFile contentsFile(destName);
            if(contentsFile.open(QIODevice::WriteOnly))
            {
                contentsFile.write(contents.readAll());
                contentsFile.close();

                ok = true;
            }
            contents.close();
        }
        zip.close();
    }

    return ok;
}

QDataStream &operator<<(QDataStream &stream, const Playlist &playlist)
{
    stream << playlist.programInfo << playlist.multimediaInfo;

    return stream;
}

QDataStream &operator>>(QDataStream &stream, Playlist &playlist)
{
    stream >> playlist.programInfo >> playlist.multimediaInfo;

    return stream;
}

void DownloadManager::loadPlaylist(Playlist *playlist)
{
    QFile file(this->path + QString("/playlist_%1.db").arg(this->fileLang));
    if(file.open(QIODevice::ReadOnly))
    {
        quint32 readMagic, version;

        QDataStream stream(&file);
        stream.setVersion(QDataStream::Qt_5_4);
        stream >> readMagic;
        stream >> version;

        if((readMagic != magic) || (version != currVersion))
            return;

        stream >> *playlist;
    }

    const QString issueFmt = "yyyyMM";
    const QDate currDate = QDate::currentDate();
    const QDate monDate = currDate.addDays(-currDate.dayOfWeek()+1);

    // Remove old entries
    QVector<ProgramInfo> newPI;
    for(int i=0; i<playlist->programInfo.size(); i++)
    {
        if(playlist->programInfo[i].pub == "w" &&
                QDate::fromString(playlist->programInfo[i].issue, issueFmt) >= monDate.addMonths(-3))
        {
            newPI.append(playlist->programInfo[i]);
        }
        else if(playlist->programInfo[i].pub == "mwb" &&
                QDate::fromString(playlist->programInfo[i].issue, issueFmt) >= monDate.addMonths(-1))
        {
            newPI.append(playlist->programInfo[i]);
        }
    }
    playlist->programInfo.swap(newPI);

    QMap<QString, QList<MultimediaInfo> > newMI;
    for(auto it=playlist->multimediaInfo.keyBegin(); it!=playlist->multimediaInfo.keyEnd(); ++it)
    {
        if(QDate::fromString(*it, "yyyyMMdd") >= monDate)
        {
            newMI[*it] = playlist->multimediaInfo[*it];
        }
    }
    playlist->multimediaInfo.swap(newMI);
}

void DownloadManager::savePlaylist(const Playlist &playlist)
{
    for(auto it=playlist.multimediaInfo.keyBegin(); it!=playlist.multimediaInfo.keyEnd(); ++it)
    {
        QDir().mkpath(this->path + "/" + *it);
    }

    QFile file(this->path + QString("/playlist_%1.db").arg(this->fileLang));
    if(file.open(QIODevice::WriteOnly))
    {
        QDataStream stream(&file);
        stream.setVersion(QDataStream::Qt_5_4);
        stream << magic;
        stream << currVersion;
        stream << playlist;
    }
}

void DownloadManager::sendStatus(const QString &date, const MultimediaInfo &info, bool ok)
{
    QNetworkAccessManager manager;
    QNetworkRequest url(QUrl(utils->getServerUrl() + "/api/set_file_status.php"));
    url.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QNetworkReply *reply = manager.post(url, QString("device=%1&user=%2&date=%3&ok=%4&sym=%5&track=%6&issue=%7&docid=%8&title=%9&filename=%10")
                                        .arg(utils->getDeviceId())
                                        .arg(utils->getUserId())
                                        .arg(date)
                                        .arg(ok)
                                        .arg(info.KeySymbol)
                                        .arg(info.Track)
                                        .arg(info.IssueTagNumber)
                                        .arg(info.MepsDocumentId)
                                        .arg(info.title) // TODO: url encode
                                        .arg(QUrl(info.url).fileName()).toUtf8());

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
}

QDataStream &operator<<(QDataStream &stream, const ProgramInfo &info)
{
    stream << info.pub << info.issue << info.checksum;

    return stream;
}

QDataStream &operator>>(QDataStream &stream, ProgramInfo &info)
{
    stream >> info.pub >> info.issue >> info.checksum;

    return stream;
}

QDataStream &operator<<(QDataStream &stream, const MultimediaInfo &info)
{
    stream << info.KeySymbol << info.Track
           << info.IssueTagNumber << info.MepsDocumentId
           << info.MimeType << info.weekend
           << info.url << info.title << info.size << info.checksum;

    return stream;
}

QDataStream &operator>>(QDataStream &stream, MultimediaInfo &info)
{
    stream >> info.KeySymbol >> info.Track
           >> info.IssueTagNumber >> info.MepsDocumentId
           >> info.MimeType >> info.weekend
           >> info.url >> info.title >> info.size >> info.checksum;

    return stream;
}
