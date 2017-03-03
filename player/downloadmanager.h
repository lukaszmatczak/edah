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

#ifndef DOWNLOADMANAGER_H
#define DOWNLOADMANAGER_H

#include <QObject>

#include <QNetworkAccessManager>
#include <QSqlDatabase>

struct ProgramInfo
{
    QString pub;
    QString issue;
    QString checksum;

    ProgramInfo() {}
    ProgramInfo(const QString &pub, const QString &issue, const QString &checksum) :
        pub(pub), issue(issue), checksum(checksum) {}

    bool operator==(const ProgramInfo &other) const
    {
        return ((this->pub == other.pub) && (this->issue == other.issue) && (this->checksum == other.checksum));
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
    QString title;
    int size;
    QString checksum;
};

struct Playlist
{
    QVector<ProgramInfo> programInfo;
    QMap<QString, QList<MultimediaInfo> > multimediaInfo;
};

struct RemoteInfo
{
    QString url;
    QString title;
    int size;
    QString checksum;
};

class DownloadManager : public QObject
{
    Q_OBJECT
public:
    explicit DownloadManager(QString path, QString videoQuality, bool signLang);

public slots:
    void start();

private:
    void removeOldFiles();
    RemoteInfo getRemoteInfo(const QString &pub, const QString &issue);
    RemoteInfo getRemoteInfo(const MultimediaInfo &minfo);
    void getRemoteMultimediaInfo(QMap<QString, QList<MultimediaInfo> > *map);
    QStringList checkFilesToDownload(const QMap<QString, QList<MultimediaInfo> > &map, int *downloadQueueBytes);
    void downloadFiles(QMap<QString, QList<MultimediaInfo> > &map, QStringList *urlsToDownload, int *downloadQueueBytes);
    bool downloadRemote(const RemoteInfo &info, QByteArray *dest);
    void downloadAndParseProgram(Playlist *playlist, const QString &pub, const QString &issue);
    bool downloadFile(MultimediaInfo *info, const QString &local);
    bool extractFile(QString zipFile, QString srcName, QString destName);

    void loadPlaylist(Playlist *playlist);
    void savePlaylist(const Playlist &playlist);

    void sendStatus(const QString &date, const MultimediaInfo &info, bool ok);

    QString path;
    QString lang;
    QString fileLang;
    QString videoQuality;

    QNetworkAccessManager *manager;
    QNetworkReply *reply;

    QSqlDatabase db;

signals:
    void setTrayText(QString text);
    void playlistLoaded(QList<MultimediaInfo>);
};

#endif // DOWNLOADMANAGER_H
