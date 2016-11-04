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

#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QObject>

#include <QNetworkAccessManager>
#include <QSqlDatabase>

struct ProgramInfo
{
    QString pub;
    QString issue;

    ProgramInfo() {}
    ProgramInfo(const QString &pub, const QString &issue) : pub(pub), issue(issue) {}

    bool operator==(const ProgramInfo &other) const
    {
        return ((this->pub == other.pub) && (this->issue == other.issue));
    }
};

struct MultimediaInfo
{
    QString KeySymbol;
    QString Track;
    QString IssueTagNumber;
    QString MepsDocumentId;
    QString MimeType;

    bool weekend;

    QString url;
    int size;
    QString checksum;
    bool downloaded;

    MultimediaInfo() : downloaded(false) {}
};

struct RemoteInfo
{
    QString url;
    int size;
    QString checksum;
};

class DownloadManager : public QObject
{
    Q_OBJECT
public:
    explicit DownloadManager(QString path, QString videoQuality);

public slots:
    void start();

private:
    RemoteInfo getRemoteInfo(const QString &pub, const QString &issue);
    RemoteInfo getRemoteInfo(const MultimediaInfo &minfo);
    bool downloadRemote(const RemoteInfo &info, QByteArray *dest);
    void downloadAndParseProgram(const QString &pub, const QString &issue);
    bool downloadFile(const MultimediaInfo &info, const QString &local);
    bool extractFile(QString zipFile, QString srcName, QString destName);

    void loadProgramInfo();
    void saveProgramInfo();

    QMap<QString, QList<MultimediaInfo> > loadMultimediaInfo();
    void saveMultimediaInfo(const QMap<QString, QList<MultimediaInfo> > &map, bool append);

    QString path;
    QString lang;
    QString videoQuality;

    QVector<ProgramInfo> programInfo;

    QStringList downloadQueue;
    int downloadQueueBytes;

    QNetworkAccessManager *manager;
    QNetworkReply *reply;

    QSqlDatabase db;

signals:
    void setTrayText(QString text);
};

#endif // DOWNLOADMANAGER_H
