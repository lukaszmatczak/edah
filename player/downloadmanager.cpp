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

#include <QEventLoop>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QSqlQuery>

#include <quazip/quazip.h>
#include <quazip/quazipfile.h>

const QString dbGetUrl = "https://www.jw.org/apps/GETPUBMEDIALINKS?output=json&fileformat=JWPUB&alllangs=0";
// &issue=201611&langwritten=P&txtCMSLang=P&pub=mwb

DownloadManager::DownloadManager(QString path) :
    path(path), QObject(nullptr), manager(nullptr)
{
    lang = tr("E");
}

void DownloadManager::start()
{
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
}

RemoteInfo DownloadManager::getRemoteInfo(const QString &pub, const QString &issue)
{
    if(!manager) manager = new QNetworkAccessManager;
    QNetworkRequest url(QUrl(dbGetUrl + QString("&langwritten=%1&txtCMSLang=%1&issue=%2&pub=%3").arg(lang).arg(issue).arg(pub)));
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

    if(info.checksum.compare(localChecksum))
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
        q.prepare("SELECT `Multimedia`.`KeySymbol`, `Multimedia`.`Track`,`Multimedia`.`IssueTagNumber`, `Multimedia`.`MepsDocumentId`, `DatedText`.`FirstDateOffset` "
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
            info.Track = q.value("Track").toInt();
            info.IssueTagNumber = q.value("IssueTagNumber").toInt();
            info.MepsDocumentId = q.value("MepsDocumentId").toInt();

            map[FirstDateOffset].append(info);
        } while(q.next());

        db.close();

        QFile(this->path + "/" + dbName).remove();

        this->saveMultimediaInfo(map);

        programInfo.append(info);
        this->saveProgramInfo();
    }
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

void DownloadManager::saveMultimediaInfo(const QMap<QString, QList<MultimediaInfo> > &map)
{
    for(auto it=map.keyBegin(); it!=map.keyEnd(); ++it)
    {
        QDir().mkpath(this->path + "/" + *it);
        QFile file(QString("%1/%2/playlist_%3.db").arg(this->path).arg(*it).arg(this->lang));
        file.open(QIODevice::ReadWrite);
        QList<MultimediaInfo> list;

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
           << info.IssueTagNumber << info.MepsDocumentId << info.weekend;

    return stream;
}

QDataStream &operator>>(QDataStream &stream, MultimediaInfo &info)
{
    stream >> info.KeySymbol >> info.Track
           >> info.IssueTagNumber >> info.MepsDocumentId >> info.weekend;

    return stream;
}
