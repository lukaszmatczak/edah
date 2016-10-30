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

#include "bigpanel.h"
#include "player.h"

#include <libedah/utils.h>

#include <QFrame>
#include <QResizeEvent>
#include <QGraphicsDropShadowEffect>
#include <QTime>
#include <QApplication>
#include <QImageReader>
#include <QFileDialog>
#include <QHeaderView>

#include <QDebug>

const QString styles = "#songInfoFrm { "
                       "  background-color: rgb(36,36,36);"
                       "  border-color:  rgb(0,0,0);"
                       "  border-top-color: rgb(70, 70, 70);"
                       "  border-left-color:  rgb(70, 70, 70);"
                       "  border-width : 1 2 2 1px;"
                       "  border-style: solid;"
                       "}"
                       "#posBar {"
                       "  background-color: rgb(36,36,36);"
                       "  border-color:  rgb(0,0,0);"
                       "  border-top-color: rgb(70, 70, 70);"
                       "  border-left-color:  rgb(70, 70, 70);"
                       "  border-width : 1 2 2 1px;"
                       "  border-style: solid;"
                       "}"
                       "#posBar::groove:horizontal {"
                       "  border-radius: 0px;"
                       "}"
                       "#posBar::sub-page:horizontal {"
                       "  background-color: rgb(0,80,255);"
                       "}"
                       "#posBar::add-page:horizontal {"
                       "}"
                       "#posBar::handle:horizontal {"
                       "  background-color: rgb(36,36,36);"
                       "  width: 1px;"
                       "}"
                       "#nonstopIcon {"
                       "  border: none;"
                       "  background: transparent;"
                       "}"
                       "#stopBtn, #keyboardBtn {"
                       "   border-radius: 0px;"
                       "}"
                       "#addWindowBtn, #removeFileBtn, #UpBtn {"
                       "   border-radius: 0px;"
                       "}"
                       "QScrollBar:vertical {"
                       "  border: 0px;"
                       "  background: rgb(36,36,36);"
                       "  width: 8px;"
                       "}"
                       "QScrollBar::handle:vertical {"
                       "  background: white;"
                       "  min-height: 10px;"
                       "  border-radius: 4px;"
                       "}"
                       "QScrollBar::handle:hover:vertical {"
                       "  background: rgb(200,200,200);"
                       " }"
                       "QScrollBar::add-line:vertical {"
                       "  background: none;"
                       "}"
                       "QScrollBar::sub-line:vertical {"
                       "  background: none;"
                       "}"
                       "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
                       "  background: none;"
                       "}"
                       "QTableView {"
                       "  font: 16px;"
                       "  selection-color: white;"
                       "  background-color: rgb(36,36,36);"
                       "  border-color:  rgb(0,0,0);"
                       "  border-bottom-color: rgb(70, 70, 70);"
                       "  border-right-color:  rgb(70, 70, 70);"
                       "  border-width : 2 1 1 2px;"
                       "  border-style: solid;"
                       "}"
                       "QTableView::item {"
                       "  border-width: 0 0 1 0px;"
                       "  border-color: black;"
                       "  border-style: dashed;"
                       "}"
                       "QTableView::item:selected {"
                       "  background-color: rgb(36,36,76);"
                       "}";

BigPanel::BigPanel(Player *player) : QWidget(0), player(player), currDuration(0)
{
    layout = new QGridLayout(this);
    this->setLayout(layout);

    QVBoxLayout *playlistLayout = new QVBoxLayout;
    layout->addLayout(playlistLayout, 1, 0, 3, 3);

    playlistView = new QTableView(this);
    playlistView->setObjectName("playlistView");
    playlistView->setSelectionMode(QAbstractItemView::SingleSelection);
    playlistView->setTextElideMode(Qt::ElideRight);
    playlistView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    playlistView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    playlistView->setShowGrid(false);
    playlistView->setSelectionBehavior(QAbstractItemView::SelectRows);
    playlistView->horizontalHeader()->setVisible(false);
    playlistView->horizontalHeader()->setStretchLastSection(true);
    playlistView->verticalHeader()->setVisible(false);
    playlistView->verticalHeader()->setMinimumSectionSize(68);
    connect(playlistView, &QTableView::doubleClicked, this, &BigPanel::playlist_dblClicked);
    playlistLayout->addWidget(playlistView);

    flick.activateOn(playlistView);

    playlistBtnArea = new QWidget;
    playlistBtnArea->setLayout(new QHBoxLayout);
    playlistBtnArea->layout()->setContentsMargins(0, 0, 0, 0);
    playlistBtnArea->layout()->setSpacing(0);
    playlistLayout->addWidget(playlistBtnArea);

    addFileBtn = new MyPushButton("", playlistBtnArea);
    addFileBtn->setObjectName("addFileBtn");
    addFileBtn->setIcon(QIcon(":/player-img/plus.svg"));
    connect(addFileBtn, &MyPushButton::pressed, this, &BigPanel::addFileBtn_clicked);
    playlistBtnArea->layout()->addWidget(addFileBtn);

    addWindowBtn = new MyPushButton("", playlistBtnArea);
    addWindowBtn->setObjectName("addWindowBtn");
    //addWindowBtn->setIcon(QIcon(":/player-img/plus.svg"));
    addWindowBtn->setEnabled(false);
    playlistBtnArea->layout()->addWidget(addWindowBtn);

    removeFileBtn = new MyPushButton("", playlistBtnArea);
    removeFileBtn->setObjectName("removeFileBtn");
    removeFileBtn->setIcon(QIcon(":/player-img/minus.svg"));
    connect(removeFileBtn, &MyPushButton::pressed, this, &BigPanel::removeFileBtn_clicked);
    playlistBtnArea->layout()->addWidget(removeFileBtn);

    UpBtn = new MyPushButton("", playlistBtnArea);
    UpBtn->setObjectName("UpBtn");
    UpBtn->setIcon(QIcon(":/player-img/arrow-up.svg"));
    connect(UpBtn, &MyPushButton::pressed, this, &BigPanel::UpBtn_clicked);
    playlistBtnArea->layout()->addWidget(UpBtn);

    DownBtn = new MyPushButton("", playlistBtnArea);
    DownBtn->setObjectName("DownBtn");
    DownBtn->setIcon(QIcon(":/player-img/arrow-down.svg"));
    connect(DownBtn, &MyPushButton::pressed, this, &BigPanel::DownBtn_clicked);
    playlistBtnArea->layout()->addWidget(DownBtn);

    QFrame *playControlsFrm = new QFrame(this);
    playControlsFrm->setObjectName("playControlsFrm");
    layout->addWidget(playControlsFrm, 0, 0, 1, 6);
    //playControlsFrm->setLayout(new QHBoxLayout);

    rndBtn = new MyPushButton("", playControlsFrm);
    rndBtn->setObjectName("rndBtn");
    rndBtn->setIcon(QIcon(":/player-img/random.svg"));
    rndBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(rndBtn, &MyPushButton::clicked, this, &BigPanel::btnRnd_clicked);

    playBtn = new MyPushButton("", playControlsFrm);
    playBtn->setIcon(QIcon(":/player-img/play.svg"));
    playBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(playBtn, &MyPushButton::clicked, this, &BigPanel::playBtn_clicked);
    //playControlsFrm->layout()->addWidget(playBtn);

    stopBtn = new MyPushButton("", playControlsFrm);
    stopBtn->setObjectName("stopBtn");
    stopBtn->setIcon(QIcon(":/player-img/stop.svg"));
    stopBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(stopBtn, &MyPushButton::pressed, this, &BigPanel::stopBtn_clicked);

    keyboardBtn = new MyPushButton("", playControlsFrm);
    keyboardBtn->setObjectName("keyboardBtn");
    keyboardBtn->setIcon(QIcon(":/player-img/keypad.svg"));
    keyboardBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(keyboardBtn, &MyPushButton::pressed, this, &BigPanel::keyboardBtn_clicked);

    playBtn->raise();

    songInfoFrm = new QFrame(playControlsFrm);
    songInfoFrm->setObjectName("songInfoFrm");
    //layout->addWidget(songInfoFrm, 0, 3, 1, 3);
    songInfoFrm->setLayout(new QHBoxLayout);

    titleLbl = new QLabel(this);
    titleLbl->setObjectName("titleLbl");
    titleLbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
    titleLbl->setWordWrap(true);
    titleLbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    songInfoFrm->layout()->addWidget(titleLbl);

    QFrame *posFrame = new QFrame(this);
    QGridLayout *posLayout = new QGridLayout(posFrame);
    posLayout->setContentsMargins(0, 0, 0, 0);
    posFrame->setLayout(posLayout);
    layout->addWidget(posFrame, 3, 3, 1, 3);

    nonstopIcon = new QPushButton(this);
    nonstopIcon->setObjectName("nonstopIcon");
    posLayout->addWidget(nonstopIcon, 0, 0);

    nonstopLbl = new QLabel(this);
    nonstopLbl->setObjectName("nonstopLbl");
    posLayout->addWidget(nonstopLbl, 0, 1);

    posLbl = new QLabel("-:--/-:--", this);
    posLbl->setObjectName("posLbl");
    posLayout->addWidget(posLbl, 0, 2, 1, 1, Qt::AlignRight);

    posBar = new Waveform(this);
    posBar->setObjectName("posBar");
    posBar->setOrientation(Qt::Horizontal);
    posBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    posBar->setEnabled(false);
    posBar->setStyle(&sliderStyle);
    connect(posBar, &Waveform::valueChanged, this, &BigPanel::posBar_valueChanged);
    connect(posBar, &Waveform::sliderReleased, this, &BigPanel::posBar_released);
    posLayout->addWidget(posBar, 1, 0, 1, 3);

    for(int i=0; i<layout->columnCount(); i++)
    {
        layout->setColumnStretch(i, 1);
    }

    for(int i=0; i<layout->rowCount(); i++)
    {
        layout->setRowStretch(i, 1);
    }

    this->setNonstop(false);
}

BigPanel::~BigPanel()
{

}

void BigPanel::addPeakMeter(PeakMeter *peakMeter)
{
    peakMeter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(peakMeter, 1, 5, 2, 1);
}

void BigPanel::removePeakMeter(PeakMeter *peakMeter)
{
    layout->removeWidget(peakMeter);
}

void BigPanel::addThumbnail(ThumbnailWidget *thumb)
{
    thumb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(thumb, 1, 3, 2, 2);
}

void BigPanel::removeThumbnail(ThumbnailWidget *thumb)
{
    layout->removeWidget(thumb);
}

void BigPanel::showEvent(QShowEvent *e)
{
    Q_UNUSED(e);

    recalcSizes(this->size());
}

void BigPanel::resizeEvent(QResizeEvent *e)
{
    this->recalcSizes(e->size());
}

void BigPanel::keyReleaseEvent(QKeyEvent *e)
{
    e->setAccepted(false);

    if((e->key() >= Qt::Key_0) && (e->key() <= Qt::Key_9))
    {
        this->showKeyboard(e->key() - Qt::Key_0);
        e->setAccepted(true);
    }
    else if(((e->key() == Qt::Key_Enter) || (e->key() == Qt::Key_Return)))
    {
        this->playBtn_clicked();
        e->setAccepted(true);
    }
    else if(e->key() == Qt::Key_BracketRight)
    {
        this->btnRnd_clicked();
        e->setAccepted(true);
    }
}

void BigPanel::retranslate()
{
    addFileBtn->setToolTip(tr("Add file to playlist"));
    addWindowBtn->setToolTip(tr("Add window to playlist"));
    removeFileBtn->setToolTip(tr("Remove selected entry from playlist"));
    UpBtn->setToolTip(tr("Move entry up"));
    DownBtn->setToolTip(tr("Move entry down"));
    rndBtn->setToolTip(tr("Play random songs continuously (<i>]</i>)"));
    playBtn->setToolTip(tr("Play (<i>Enter</i>)"));
    stopBtn->setToolTip(tr("Stop"));
    keyboardBtn->setToolTip(tr("Open keyboard and play manually selected song (<i>0-9</i>)"));
    nonstopLbl->setText(tr("Autoplay"));
}

void BigPanel::changeEvent(QEvent *e)
{
    if(e->type() == QEvent::LanguageChange)
    {
        this->retranslate();
    }
    else
    {
        QWidget::changeEvent(e);
    }
}

void BigPanel::recalcSizes(const QSize &size)
{
    layout->setSpacing((size.width()+size.height())/128);

    playlistView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    int s = playBtn->parentWidget()->height();

    rndBtn->setGeometry(0, s*0.2f, size.width()*0.13f, s*0.6f);
    rndBtn->setIconSize(QSize(s/3, s/3));

    QSize playBtnSize(s, s);
    playBtn->setGeometry(size.width()*0.10f, 0, s, s);

    QSize iconSize = QSize(playBtn->width(), playBtn->height());
    playBtn->setIconSize(playBtnSize);

    QRegion mask(0, 0, playBtnSize.width(), playBtnSize.height(), QRegion::Ellipse);
    playBtn->setMask(mask);

    int x = size.width()*0.07f+s;
    stopBtn->setGeometry(x, s*0.2f, s*0.9f, s*0.6f);
    stopBtn->setIconSize(QSize(s/3, s/3));
    x += s*0.9f;

    keyboardBtn->setGeometry(x, s*0.2f, s*0.9f, s*0.6f);
    keyboardBtn->setIconSize(QSize(s/3, s/3));
    x += s*0.9f;

    songInfoFrm->setGeometry(x, s*0.2f, songInfoFrm->parentWidget()->width()-x, s*0.6f);

    playlistBtnArea->setFixedHeight(size.height()/12);

    iconSize = QSize(size.height()/12, size.height()/12);
    addFileBtn->setIconSize(iconSize);
    addWindowBtn->setIconSize(iconSize);
    removeFileBtn->setIconSize(iconSize);
    UpBtn->setIconSize(iconSize);
    DownBtn->setIconSize(iconSize);
    keyboardBtn->setIconSize(iconSize);

    this->setStyleSheet(styles +  QString(
                            ".QPushButton, #numberLbl {"
                            "   font-size: %1px;"
                            "   font-weight: 900;"
                            "}"
                            "#titleLbl {"
                            "   font-size: %2px;"
                            "   color: white;"
                            "}"
                            "#posLbl, #nonstopLbl {"
                            "   font-size: %3px;"
                            "}"
                            "#rndBtn {"
                            "   border-top-left-radius: %4px;"
                            "   border-bottom-left-radius: %4px;"
                            "}"
                            "#songInfoFrm {"
                            "   border-top-left-radius: 0px;"
                            "   border-bottom-left-radius: 0px;"
                            "   border-top-right-radius: %4px;"
                            "   border-bottom-right-radius: %4px;"
                            "}"
                            "#addFileBtn {"
                            "   border-top-left-radius: %5px;"
                            "   border-bottom-left-radius: %5px;"
                            "   border-top-right-radius: 0px;"
                            "   border-bottom-right-radius: 0px;"
                            "}"
                            "#DownBtn {"
                            "   border-top-left-radius: 0;"
                            "   border-bottom-left-radius: 0px;"
                            "   border-top-right-radius: %5px;"
                            "   border-bottom-right-radius: %5px;"
                            "}")
                        .arg(qMax(1, size.height()/16))
                        .arg(qMax(1, size.height()/32))
                        .arg(qMax(1, size.height()/24))
                        .arg(qMax(1, s/4))
                        .arg(qMax(1, size.height()/24)));

    iconSize = QSize(size.height()/24, size.height()/24);
    nonstopIcon->setIconSize(iconSize);
    nonstopIcon->setFixedSize(iconSize);
}

void BigPanel::playlist_dblClicked(const QModelIndex &index)
{
    if(!player->isPlaying())
    {
        player->playlistModel.setCurrentItem(index.row());
        this->setCurrentPlaylistEntry(player->playlistModel.getCurrentItem());
    }

    //mpv->setPause(true);
}

void BigPanel::addFileBtn_clicked()
{
    settings->beginGroup(player->getPluginId());

    QString imgFilter;
    foreach (QByteArray ext, QImageReader::supportedImageFormats())
    {
        imgFilter += "*." + ext + " ";
    }
    imgFilter = imgFilter.left(imgFilter.length()-1);

    QStringList filenames = QFileDialog::getOpenFileNames(0,
                                                    tr("Choose files"),
                                                    settings->value("lastDir").toString(),
                                                    tr("All supported files") + " (*.rmvb *.mpeg *.mpg *.mpe *.m1v *.m2v *.mp4 *.mp4v *.mpg4 *.avi *.ogv *.qt *.mov *.asf *.asx *.wmv *.wmx *.fli *.flv *.mkv *.mk3d *.mks *.webm *.aac *.mpga *.mp2 *.mp2a *.mp3 *.m2a *.m3a *.oga *.ogg *.spx *.ram *.ra *.rmp *.wma *.wav *.flac *.m4a *.mp4a " + imgFilter + ");;" +
                                                          tr("Video") + " (*.rmvb *.mpeg *.mpg *.mpe *.m1v *.m2v *.mp4 *.mp4v *.mpg4 *.avi *.ogv *.qt *.mov *.asf *.asx *.wmv *.wmx *.fli *.flv *.mkv *.mk3d *.mks *.webm);;" +
                                                          tr("Audio") +" (*.aac *.mpga *.mp2 *.mp2a *.mp3 *.m2a *.m3a *.oga *.ogg *.spx *.ram *.ra *.rmp *.wma *.wav *.flac *.m4a *.mp4a);;" +
                                                          tr("Images") + " (" + imgFilter + ");;" +
                                                          tr("All files (*.*)"));

    if(!filenames.isEmpty())
    {
        settings->setValue("lastDir", QFileInfo(filenames[0]).dir().absolutePath());

        for(int i=0; i<filenames.size(); i++)
            player->playlistModel.addFile(filenames[i]);
    }

    settings->endGroup();
}

void BigPanel::removeFileBtn_clicked()
{
    int curr = playlistView->currentIndex().row();

    if(player->playlistModel.getCurrentItem() != curr || !player->isPlaying())
    {
        player->playlistModel.removeEntry(curr);
        this->setCurrentPlaylistEntry(-1);
    }
}

void BigPanel::UpBtn_clicked()
{
    int currRow = playlistView->currentIndex().row();
    if(currRow < 1) return;

    player->playlistModel.swapEntries(currRow, currRow-1);
    QModelIndex idx = player->playlistModel.index(currRow-1, 0, QModelIndex());
    playlistView->setCurrentIndex(idx);
}

void BigPanel::DownBtn_clicked()
{
    int currRow = playlistView->currentIndex().row();
    if(currRow >= player->playlistModel.rowCount(QModelIndex())-1) return;

    player->playlistModel.swapEntries(currRow, currRow+1);
    QModelIndex idx = player->playlistModel.index(currRow+1, 0, QModelIndex());
    playlistView->setCurrentIndex(idx);
}

void BigPanel::btnRnd_clicked()
{
    if (player->isPlaying())
        return;

    this->setNonstop(true);

    emit playSong(-1, true);
}

void BigPanel::keyboardBtn_clicked()
{
    this->showKeyboard(0);
}

void BigPanel::showKeyboard(int number)
{
    Keypad keyboardPopup(number, player, this);
    connect(&keyboardPopup, &Keypad::songEntered, this, [this](int number) {
        this->setNonstop(false);
        emit playSong(number, nonstop);
    });
    keyboardPopup.setSize(0.42f, 0.9f); // TODO
    keyboardPopup.exec();
}

bool BigPanel::event(QEvent *e)
{
    if(e->type() == QEvent::WindowActivate)
    {
        player->updateThumbnailPos();
    }

    e->ignore();

    return QWidget::event(e);
}

void BigPanel::setCurrentPlaylistEntry(int n)
{
    if(n >= 0)
    {
        player->playlistModel.setCurrentItem(n);
        QModelIndex index = player->playlistModel.index(player->playlistModel.getCurrentItem(), 0, QModelIndex());
        playlistView->setCurrentIndex(index);
        playlistView->scrollTo(index);
        playlistView->repaint();
    }

    titleLbl->setText(player->playlistModel.getCurrentItemInfo().title);
    posBar->setWaveform(player->playlistModel.getCurrentItemInfo().waveform);
}

void BigPanel::playBtn_clicked()
{
    int currItem = playlistView->currentIndex().row();
    if(currItem < 0)
        currItem = player->playlistModel.getCurrentItem();

    if(currItem >= player->playlistModel.rowCount(QModelIndex()))
        currItem = 0;

    emit play(currItem);
}

void BigPanel::stopBtn_clicked()
{
    this->setNonstop(false);
    emit stop();
}

void BigPanel::setNonstop(bool isSet)
{
    nonstop = isSet;

    nonstopIcon->setIcon(QIcon(isSet ? ":/player-img/nonstop-on.svg" : ":/player-img/nonstop-off.svg"));
    nonstopLbl->setStyleSheet(isSet ? "color: rgb(0, 80, 255);" : "color: rgb(30, 40, 45);");
}

void BigPanel::playerPositionChanged(bool paused, double pos, double duration)
{
    static bool prevPaused = true;

    if(prevPaused != paused)
    {
        playBtn->setIcon(QIcon(paused ? ":/player-img/play.svg" : ":/player-img/pause.svg"));
        playBtn->setToolTip(paused ? tr("Play (<i>Enter</i>)") : tr("Pause (<i>Enter</i>)"));
        prevPaused = paused;
    }

    currDuration = duration;

    if(!posBar->isSliderDown())
    {
        QString posStr = "-:--";
        QString durationStr = "/-:--";
        if(pos > -1) posStr = QTime(0, 0).addSecs(pos).toString("m:ss");
        if(duration > -1) durationStr = QTime(0, 0).addSecs(duration).toString("/m:ss");
        posLbl->setText(posStr + durationStr);

        posBar->setValue(pos>0 ? pos*10 : 0);
        posBar->setMaximum(duration>0 ? duration*10 : 1);
    }

    posBar->setEnabled(duration > 0.0);
}

void BigPanel::posBar_valueChanged(int value)
{
    QString posStr = "-:--";
    QString durationStr = "/-:--";
    if(value > -1) posStr = QTime(0, 0).addSecs(value/10).toString("m:ss");
    if(currDuration > -1) durationStr = QTime(0, 0).addSecs(currDuration).toString("/m:ss");
    posLbl->setText(posStr + durationStr);
}

void BigPanel::posBar_released()
{
    emit seek(posBar->value()*100);
}
