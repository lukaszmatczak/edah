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

#include "playlistmodel.h"

#include <libedah/logger.h>

#include <QFileInfo>
#include <QDir>
#include <QFont>
#include <QJsonArray>
#include <QDebug>
#include <QImageReader>
#include <QThreadPool>

#include <bass.h>

#include <taglib/tag.h>
#include <taglib/fileref.h>
#include <taglib/mp4file.h>
#include <taglib/mp4tag.h>
#include <taglib/mp4coverart.h>
#include <taglib/tpropertymap.h>
#include <taglib/tmap.h>
#include <taglib/tlist.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>


#if defined(_MSC_VER)
#undef min
#undef max
#endif

PlaylistModel::PlaylistModel() : currItem(0)
{
    connect(&timer, &QTimer::timeout, this, &PlaylistModel::updateEntries);
    timer.start(1000);
}

PlaylistModel::~PlaylistModel()
{

}

QModelIndex PlaylistModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return createIndex(row, column);
}

QModelIndex PlaylistModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child);
    return QModelIndex();
}

int PlaylistModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return entries.size();
}

int PlaylistModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 2;
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    if(index.column() == 0)
    {
        if(role == Qt::DecorationRole)
        {
            return entries[index.row()].thumbnail;
        }
    }
    else if(index.column() == 1)
    {
        if(role == Qt::DisplayRole)
        {
            return entries[index.row()].title;
        }
        else if(role == Qt::ToolTipRole)
        {
            QString tooltip = entries[index.row()].title;
            if(!entries[index.row()].filename.isEmpty())
            {
                tooltip += "\n" + QDir::toNativeSeparators(entries[index.row()].filename);
            }

            int flags = entries[index.row()].flags;
            if(flags) tooltip += "\nOpcje:";
            if(flags & EF_WIN_SCALE)      tooltip += "\n  Skalowanie";
            if(flags & EF_WIN_WITHCURSOR) tooltip += "\n  Pokazywanie kursora myszy";

            return tooltip;
        }
        else if(role == Qt::FontRole)
        {
            QFont font;
            if(index.row() == currItem && currFile.type == EntryInfo::Empty)
                font.setBold(true);

            if(!entries[index.row()].exists)
                font.setStrikeOut(true);

            return font;
        }
    }
    return QVariant();
}

void PlaylistModel::setCurrentFile(QString filename)
{
    currFile.filename = filename;
    currFile.type = EntryInfo::AV;
    currFile.thumbnail = QPixmap(64, 64);
    currFile.thumbnail.fill(Qt::red);

#ifdef Q_OS_WIN
    TagLib::FileRef filetag(filename.toStdWString().c_str());
#else
    TagLib::FileRef filetag(filename.toStdString().c_str());
#endif
    TagLib::Tag *tag = filetag.tag();
#ifdef Q_OS_WIN
    if(tag) currFile.title = QString::fromUtf16((ushort*)tag->title().toCWString());
#else
    if(tag) currFile.title = QString::fromUtf8(tag->title().toCString(true));
#endif
    if(currFile.title.isEmpty()) currFile.title = QFileInfo(filename).fileName();

    TagLib::AudioProperties *audioProperties = filetag.audioProperties();
    if(audioProperties) currFile.duration = audioProperties->length();
    currFile.exists = QFileInfo(filename).exists();

    // Waveforms
    SongInfoWorker *worker = new SongInfoWorker(-1, filename);
    connect(worker, &SongInfoWorker::done, this, [this](int id, QByteArray waveform) {
        if(currFile.type == EntryInfo::AV)
        {
            currFile.waveform = waveform;
            emit waveformChanged();
        }
    });

    QThreadPool::globalInstance()->start(worker);
}

void PlaylistModel::addFile(QString filename)
{
    emit layoutAboutToBeChanged();

    EntryInfo entry;
    entry.filename = filename;
    entry.type = EntryInfo::AV;
    entry.thumbnail = QPixmap(64, 64);
    entry.thumbnail.fill(Qt::red);

    if(filename.endsWith(".mp4")) //TODO
    {
        TagLib::MP4::File mp4file(filename.toStdWString().c_str());

        TagLib::MP4::Tag *tag = mp4file.tag();
        if(tag)
        {
            TagLib::MP4::CoverArtList covers = tag->itemListMap()["covr"].toCoverArtList();
            if(covers.size() > 0)
            {
                TagLib::ByteVector cover = covers.front().data();
                QImage img = QImage::fromData((uchar*)cover.data(), cover.size());
                entry.thumbnail = QPixmap::fromImage(img.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        }
    }
    else if(filename.endsWith(".mp3")) // TODO
    {
        TagLib::MPEG::File mp3file(filename.toStdWString().c_str());

        TagLib::ID3v2::Tag *tag = mp3file.ID3v2Tag();
        if(tag)
        {
            TagLib::ID3v2::FrameList frames = tag->frameListMap()["APIC"];
            if(frames.size() > 0)
            {
                TagLib::ID3v2::AttachedPictureFrame *frame = (TagLib::ID3v2::AttachedPictureFrame*)frames.front();
                QImage img = QImage::fromData((uchar*)frame->picture().data(), frame->picture().size());
                entry.thumbnail = QPixmap::fromImage(img.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        }
    }

#ifdef Q_OS_WIN
    TagLib::FileRef filetag(filename.toStdWString().c_str());
#else
    TagLib::FileRef filetag(filename.toStdString().c_str());
#endif
    TagLib::Tag *tag = filetag.tag();
#ifdef Q_OS_WIN
    if(tag) entry.title = QString::fromUtf16((ushort*)tag->title().toCWString());
#else
    if(tag) entry.title = QString::fromUtf8(tag->title().toCString(true));
#endif
    if(entry.title.isEmpty()) entry.title = QFileInfo(filename).fileName();

    TagLib::AudioProperties *audioProperties = filetag.audioProperties();
    if(audioProperties) entry.duration = audioProperties->length();
    entry.exists = QFileInfo(filename).exists();

    foreach (QByteArray ext, QImageReader::supportedImageFormats())
    {
        if(filename.endsWith("."+ext, Qt::CaseInsensitive))
            entry.type = EntryInfo::Image;
    }

    entries.push_back(entry);

    // Waveforms
    //QString filepath = songsDir.filePath(songs[keys[i]].filename);
    SongInfoWorker *worker = new SongInfoWorker(entries.size()-1, filename);
    connect(worker, &SongInfoWorker::done, this, [this](int id, QByteArray waveform) {
        entries[id].waveform = waveform;

        if(id == this->currItem)
            emit waveformChanged();
    });

    QThreadPool::globalInstance()->start(worker);

    emit layoutChanged();
    emit dataChanged(createIndex(entries.size()-1, 0), createIndex(entries.size()-1, 0)); // TODO: ???
}

void PlaylistModel::addWindow(WId winID, int flags)
{
    emit layoutAboutToBeChanged();

    EntryInfo entry;
    entry.type = EntryInfo::Window;
    entry.exists = true;
    entry.winID = winID;
    entry.flags = flags;
    entry.title = QString("Okno \"") + /*utils->getWindowTitle(winID) +*/ "\""; // TODO

    entries.push_back(entry);

    emit layoutChanged();
    emit dataChanged(createIndex(entries.size()-1, 0), createIndex(entries.size()-1, 0)); // TODO: ???
}

void PlaylistModel::removeEntry(int pos)
{
    emit layoutAboutToBeChanged();

    if(!(pos < 0 || pos >= entries.size()))
        entries.remove(pos);

    emit layoutChanged();
}

void PlaylistModel::swapEntries(int pos1, int pos2)
{
    emit layoutAboutToBeChanged();

    qSwap(entries[pos1], entries[pos2]);

    if(this->getCurrentItem() == pos1)
        this->setCurrentItem(pos2);
    else if(this->getCurrentItem() == pos2)
        this->setCurrentItem(pos1);

    emit layoutChanged();
}

EntryInfo PlaylistModel::getItemInfo(int n)
{
    if(n < 0 || n >= entries.size())
        return EntryInfo();

    return entries[n];
}

EntryInfo PlaylistModel::getCurrentItemInfo()
{
    if(currFile.type == EntryInfo::AV)
        return currFile;

    return getItemInfo(this->currItem);
}

void PlaylistModel::setCurrentItem(int n)
{
    emit layoutAboutToBeChanged();
    int prev = this->currItem;

    this->currItem = n;
    currFile.type = EntryInfo::Empty;

    emit dataChanged(createIndex(prev, 0), createIndex(prev, 0));
    emit dataChanged(createIndex(n, 0), createIndex(n, 0));

    emit layoutChanged();
}

int PlaylistModel::getCurrentItem()
{
    return this->currItem;
}

void PlaylistModel::nextItem()
{
    int nextItem = this->currItem;

    if(currFile.type == EntryInfo::Empty)
        nextItem++;

    this->setCurrentItem(nextItem);
}

void PlaylistModel::updateEntries()
{
/*    for(int i=0; i<entries.size(); i++)
    {
        if(entries[i].type == EntryInfo::Window)
        {
            QString newTitle = utils->getWindowTitle(entries[i].winID);
            if(newTitle.isNull())
            {
                entries[i].exists = false;
            }
            else
            {
                entries[i].title = "Okno \"" + newTitle + "\"";
            }

            emit dataChanged(createIndex(i, 0), createIndex(i, 0));
        }
    }*/ // TODO
}

//////////////////////
/// SongInfoWorker ///
//////////////////////

SongInfoWorker::SongInfoWorker(int id, QString filepath) : number(id), filepath(filepath)
{

}

void SongInfoWorker::run()
{
    HSTREAM stream;
    QByteArray form;

    mutex.lock();
    stream = BASS_StreamCreateFile(FALSE,
                                   filepath.utf16(),
                                   0,
                                   0,
                                   BASS_STREAM_DECODE | BASS_SAMPLE_MONO | BASS_STREAM_PRESCAN | BASS_SAMPLE_FLOAT | BASS_UNICODE);
    mutex.unlock();

    if(!stream)
    {
        LOG(QString("Cannot create waveform of file \"%1\" (error %2)")
            .arg(filepath)
            .arg(BASS_ErrorGetCode()));
        return;
    }

    QWORD length = BASS_ChannelGetLength(stream, BASS_POS_BYTE);
    float *buf = new float[length/1024+1];

    for(int i=0; i<1024; i++)
    {
        int count = BASS_ChannelGetData(stream, buf, (length/1024) | BASS_DATA_FLOAT);

        qint8 maxi = std::numeric_limits<qint8>::min();
        qint8 mini = std::numeric_limits<qint8>::max();

        for(int i=0; i<count/4; i++)
        {
            maxi = qMax<qint8>(maxi, buf[i]*127);
            mini = qMin<qint8>(mini, buf[i]*127);
        }

        form.append(maxi+127);
        form.append(mini+127);
    }

    BASS_StreamFree(stream);
    delete [] buf;

    emit done(number, form);
}

bool SongInfoWorker::autoDelete()
{
    return true;
}
