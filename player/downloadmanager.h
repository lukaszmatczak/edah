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
    int Track;
    int IssueTagNumber;
    int MepsDocumentId;

    bool weekend;
    //QString FirstDateOffset;
};

struct RemoteInfo
{
    QString url;
    int size;
    QString checksum; // md5sum
};

class DownloadManager : public QObject
{
    Q_OBJECT
public:
    explicit DownloadManager(QString path);

public slots:
    void start();

private:
    RemoteInfo getRemoteInfo(const QString &pub, const QString &issue);
    bool downloadRemote(const RemoteInfo &info, QByteArray *dest);
    void downloadAndParseProgram(const QString &pub, const QString &issue);
    bool extractFile(QString zipFile, QString srcName, QString destName);

    void loadProgramInfo();
    void saveProgramInfo();

    void saveMultimediaInfo(const QMap<QString, QList<MultimediaInfo> > &map);

    QString path;
    QString lang;

    QVector<ProgramInfo> programInfo;

    QNetworkAccessManager *manager;
    QNetworkReply *reply;

    QSqlDatabase db;
};

#endif // DOWNLOADMANAGER_H
