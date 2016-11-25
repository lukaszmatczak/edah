/*
    Edah
    Copyright (C) 2015-2016  Lukasz Matczak

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

#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QAbstractItemModel>
#include <QWidget>
#include <QTimer>
#include <QRunnable>
#include <QMutex>
#include <QFileInfo>

#define EF_WIN_SCALE      (1 << 0)
#define EF_WIN_WITHCURSOR (1 << 1)

struct EntryInfo
{
    enum Type {Empty, AV, Image, Window, Keypad};
    Type type;
    QString title;
    int duration;
    bool exists;
    QPixmap thumbnail;
    QByteArray waveform;

    // Type: AV, Image
    QString filename;

    // Type: Window
    WId winID;
    int flags;

    EntryInfo()
    {
        type = Empty;
        duration = 0;
        exists = false;
        winID = 0;
        flags = 0;
    }
};

class SongInfoWorker : public QObject, public QRunnable
{
    Q_OBJECT
public:
    SongInfoWorker(QString filepath);

    void run();
    bool autoDelete();

private:
    QString filepath;
    QMutex mutex;

signals:
    void done(QString filepath, QByteArray waveform);
};

class PlaylistModel : public QAbstractItemModel
{
Q_OBJECT
public:
    PlaylistModel();
    ~PlaylistModel();

    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;

    template<typename T> QPixmap getCover(const QFileInfo &finfo, const T &filename);
    void addFile(QString filename, int position = -1);
    void setCurrentFile(QString filename);
    void addWindow(WId winID, int flags);
    QModelIndex addKeypad();
    void removeEntry(int pos);
    void swapEntries(int pos1, int pos2);

    void setSong(const QString &filename);
    EntryInfo getItemInfo(int n);
    EntryInfo getCurrentItemInfo();
    void setCurrentItem(int n);
    int getCurrentItem();
    void nextItem();

    EntryInfo currFile;

private:
    static QString PlaylistModel::getWindowTitle(WId winID);
    static QPixmap PlaylistModel::getWindowIcon(WId winID);

    QVector<EntryInfo> entries;
    int currItem;
    QTimer timer;

private slots:
    void updateEntries();

signals:
    void waveformChanged();
};

#endif // PLAYLISTMODEL_H
