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

#include "downloadmanager.h"

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

#include <QSqlQuery>

#include <quazip/quazip.h>
#include <quazip/quazipfile.h>

const QString dbGetUrl = "https://www.jw.org/apps/GETPUBMEDIALINKS?output=json&alllangs=0";
// &issue=201611&langwritten=P&txtCMSLang=P&pub=mwb&fileformat=JWPUB

DownloadManager::DownloadManager(QString path, QString videoQuality) :
    path(path), videoQuality(videoQuality), QObject(nullptr), manager(nullptr),
    downloadQueueBytes(0)
{
    lang = tr("E");
}

void DownloadManager::start()
{
    emit setTrayText(tr("Downloading"));

    this->db = QSqlDatabase::addDatabase("QSQLITE");

    QDir().mkpath(this->path);

    this->loadProgramInfo();

    const QString issueFmt = "yyyyMM";
    const QDate currDate = QDate::currentDate();
    const QDate monDate = currDate.addDays(-currDate.dayOfWeek()+1);

    this->downloadAndParseProgram("mwb", monDate.toString(issueFmt));
    this->downloadAndParseProgram("w", monDate.addMonths(-2).toString(issueFmt));

    this->downloadAndParseProgram("mwb", monDate.addMonths(1).toString(issueFmt));
    this->downloadAndParseProgram("w", monDate.addMonths(-1).toString(issueFmt));

    QMap<QString, QList<MultimediaInfo> > map = this->loadMultimediaInfo();

    QStringList songs;
    songs << "iasn" << "iasnm" << "snnw" << "sjjm";

    for(auto it=map.begin(); it!=map.end(); ++it)
    {
        for(int i=0; i<it->size(); i++)
        {
            if(!songs.contains(it->at(i).KeySymbol) && it->at(i).url.isEmpty()) // TODO: video quality change
            {
                RemoteInfo info = this->getRemoteInfo(it->at(i));

                if(info.url.isEmpty())
                {
                    // TODO: err
                }
                else
                {
                    (*it)[i].url = info.url;
                    (*it)[i].size = info.size;
                    (*it)[i].checksum = info.checksum;
                }
            }
        }
    }

    this->saveMultimediaInfo(map, false);

    int toDownload = 0;
    for(auto it=map.begin(); it!=map.end(); ++it)
    {
        for(int i=0; i<it->size(); i++)
        {
            if(!it->at(i).downloaded && !it->at(i).url.isEmpty())
            {
                toDownload++;
                downloadQueueBytes += it->at(i).size;
            }
        }
    }

    emit setTrayText(tr("Downloading") + tr("\n%n file(s)", "", toDownload) + QString(" (%1 MB) left").arg(downloadQueueBytes/(1024*1024)));

    for(auto it=map.keyBegin(); it!=map.keyEnd(); ++it)
    {
        for(int i=0; i<map[*it].size(); i++)
        {
            if(!map[*it][i].downloaded && !map[*it][i].url.isEmpty())
            {
                if(this->downloadFile(map[*it][i], this->path + "/" + *it + "/" + QUrl(map[*it][i].url).fileName()))
                {
                    toDownload--;
                    downloadQueueBytes -= map[*it][i].size;
                    map[*it][i].downloaded = true;

                    this->saveMultimediaInfo(map, false);

                    emit setTrayText(tr("Downloading") + tr("\n%n file(s)", "", toDownload) + QString(" (%1 MB) left").arg(downloadQueueBytes/(1024*1024)));
                }
                else
                {
                    // TODO: err
                }
            }
        }
    }

    emit setTrayText("");
}

RemoteInfo DownloadManager::getRemoteInfo(const QString &pub, const QString &issue)
{
    if(!manager) manager = new QNetworkAccessManager;
    QNetworkRequest url(QUrl(dbGetUrl + QString("&langwritten=%1&txtCMSLang=%1&issue=%2&pub=%3&fileformat=JWPUB")
                             .arg(lang)
                             .arg(issue)
                             .arg(pub)));
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
                             .arg(lang)
                             .arg(minfo.IssueTagNumber)
                             .arg(minfo.KeySymbol)
                             .arg(minfo.MepsDocumentId)));
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
                .value(lang).toObject();

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
                info.checksum = file["file"].toObject()["checksum"].toString();
                info.size = file["filesize"].toInt();

                return info;
            }
        }
    }

    return info;
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

void DownloadManager::downloadAndParseProgram(const QString &pub, const QString &issue)
{
    ProgramInfo info(pub, issue);

    if(!programInfo.contains(info))
    {
        RemoteInfo remoteInfo = this->getRemoteInfo(pub, issue);

        if(remoteInfo.url.isEmpty())
            return;

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

        QMap<QString, QList<MultimediaInfo> > map;

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

            map[FirstDateOffset].append(info);
        } while(q.next());

        db.close();

        QFile(this->path + "/" + dbName).remove();

        this->saveMultimediaInfo(map, true);

        programInfo.append(info);
        this->saveProgramInfo();
    }
}

bool DownloadManager::downloadFile(const MultimediaInfo &info, const QString &local)
{
    if(!manager) manager = new QNetworkAccessManager;
    QNetworkRequest request(QUrl{info.url});
    reply = manager->get(request);

    QFile file(local);
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

    if(info.checksum.compare(hash.result().toHex(), Qt::CaseInsensitive))
    {
        return false;
    }

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

void DownloadManager::loadProgramInfo()
{
    QFile file(this->path + QString("/program_%1.db").arg(this->lang));
    if(file.open(QIODevice::ReadOnly))
    {
        QDataStream stream(&file);
        stream >> programInfo;
    }
}

void DownloadManager::saveProgramInfo()
{
    QFile file(this->path + QString("/program_%1.db").arg(this->lang));
    if(file.open(QIODevice::WriteOnly))
    {
        QDataStream stream(&file);
        stream << programInfo;
    }
}

QMap<QString, QList<MultimediaInfo> > DownloadManager::loadMultimediaInfo()
{
    QMap<QString, QList<MultimediaInfo> > map;

    QDir dir(this->path);
    QStringList dirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

    for(int i=0; i<dirs.size(); i++)
    {
        QFile file(QString("%1/%2/playlist_%3.db").arg(this->path).arg(dirs[i]).arg(this->lang));
        file.open(QIODevice::ReadOnly);

        QList<MultimediaInfo> list;

        QDataStream stream(&file);
        stream >> list;

        map[dirs[i]] = list;

        file.close();
    }

    return map;
}

void DownloadManager::saveMultimediaInfo(const QMap<QString, QList<MultimediaInfo> > &map, bool append)
{
    for(auto it=map.keyBegin(); it!=map.keyEnd(); ++it)
    {
        QDir().mkpath(this->path + "/" + *it);
        QFile file(QString("%1/%2/playlist_%3.db").arg(this->path).arg(*it).arg(this->lang));
        file.open(QIODevice::ReadWrite);
        QList<MultimediaInfo> list;

        if(append)
        {
            QDataStream stream(&file);
            stream >> list;
        }

        list += map[*it];
        file.resize(0);

        {
            QDataStream stream(&file);
            stream << list;
        }

        file.close();
    }
}

QDataStream &operator<<(QDataStream &stream, const ProgramInfo &info)
{
    stream << info.pub << info.issue;

    return stream;
}

QDataStream &operator>>(QDataStream &stream, ProgramInfo &info)
{
    stream >> info.pub >> info.issue;

    return stream;
}

QDataStream &operator<<(QDataStream &stream, const MultimediaInfo &info)
{
    stream << info.KeySymbol << info.Track
           << info.IssueTagNumber << info.MepsDocumentId
           << info.MimeType << info.weekend
           << info.url << info.size << info.checksum
           << info.downloaded;

    return stream;
}

QDataStream &operator>>(QDataStream &stream, MultimediaInfo &info)
{
    stream >> info.KeySymbol >> info.Track
           >> info.IssueTagNumber >> info.MepsDocumentId
           >> info.MimeType >> info.weekend
           >> info.url >> info.size >> info.checksum
           >> info.downloaded;

    return stream;
}
