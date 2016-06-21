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

#include <QFrame>
#include <QResizeEvent>
#include <QGraphicsDropShadowEffect>
#include <QTime>
#include <QApplication>

#include <QDebug>

BigPanel::BigPanel(Player *player) : QWidget(0), player(player), currDuration(0)
{
    layout = new QGridLayout(this);
    this->setLayout(layout);

    rndPlaylist = new ShufflePlaylist(&player->songs);

    rndBtn = new MyPushButton("", this);
    rndBtn->setIcon(QIcon(":/player-img/random.svg"));
    rndBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(rndBtn, &MyPushButton::clicked, this, &BigPanel::btnRnd_clicked);
    layout->addWidget(rndBtn, 3, 0);

    MyPushButton *btn0 = new MyPushButton("0", this);
    btn0->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(btn0, 3, 1);
    numberBtns.push_back(btn0);

    btnBack = new MyPushButton("", this);
    btnBack->setIcon(QIcon(":/player-img/back.svg"));
    btnBack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(btnBack, &MyPushButton::clicked, this, &BigPanel::btnBack_clicked);
    layout->addWidget(btnBack, 3, 2);

    for(int i=0; i<9; i++)
    {
        MyPushButton *btn = new MyPushButton(QString::number(i+1), this);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        layout->addWidget(btn, i/3, i%3);
        numberBtns.push_back(btn);
    }

    for(int i=0; i<numberBtns.size(); i++)
    {
        connect(numberBtns[i], &MyPushButton::clicked, this, &BigPanel::numberBtn_clicked);
    }

    QFrame *songInfoFrm = new QFrame(this);
    songInfoFrm->setObjectName("songInfoFrm");
    layout->addWidget(songInfoFrm, 0, 3, 1, 3);
    songInfoFrm->setLayout(new QHBoxLayout);

    numberLbl = new QLabel(this);
    numberLbl->setObjectName("numberLbl");
    numberLbl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Ignored);
    songInfoFrm->layout()->addWidget(numberLbl);

    titleLine = new QWidget(this);
    titleLine->setObjectName("titleLine");
    titleLine->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    titleLine->setFixedWidth(1);
    titleLine->hide();
    songInfoFrm->layout()->addWidget(titleLine);

    titleLbl = new QLabel(this);
    titleLbl->setObjectName("titleLbl");
    titleLbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
    titleLbl->setWordWrap(true);
    titleLbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    songInfoFrm->layout()->addWidget(titleLbl);

    playBtn = new MyPushButton("", this);
    playBtn->setIcon(QIcon(":/player-img/play.svg"));
    playBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(playBtn, &MyPushButton::clicked, this, &BigPanel::playBtn_clicked);
    layout->addWidget(playBtn, 1, 3, 2, 2);

    QFrame *posFrame = new QFrame(this);
    QGridLayout *posLayout = new QGridLayout(posFrame);
    posFrame->setLayout(posLayout);
    layout->addWidget(posFrame, 3, 3, 1, 3);

    nonstopIcon = new QPushButton(this);
    nonstopIcon->setObjectName("nonstopIcon");
    posLayout->addWidget(nonstopIcon, 0, 0);

    nonstopLbl = new QLabel(tr("Autoplay"), this);
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
    delete rndPlaylist;
}

void BigPanel::addPeakMeter(PeakMeter *peakMeter)
{
    peakMeter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(peakMeter, 1, 5, 2, 1);
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
        this->addDigit(e->key() - Qt::Key_0);
        e->setAccepted(true);
    }
    else if(e->key() == Qt::Key_Backspace)
    {
        this->btnBack_clicked();
        e->setAccepted(true);
    }
    else if(((e->key() == Qt::Key_Enter) || (e->key() == Qt::Key_Return)) && !player->isPlaying())
    {
        this->playBtn_clicked();
        e->setAccepted(true);
    }
    else if(e->key() == Qt::Key_Backslash)
    {
        this->btnRnd_clicked();
        e->setAccepted(true);
    }
}

void BigPanel::recalcSizes(const QSize &size)
{
    layout->setSpacing((size.width()+size.height())/128);

    QFontMetrics fm = QFontMetrics(numberLbl->font());
    numberLbl->setFixedWidth(fm.width("000"));
    titleLine->setFixedHeight(fm.height());

    this->setStyleSheet(QString("#songInfoFrm { "
                                "background-color: rgb(40,40,40);"
                                "border-color:  rgb(0,0,0);"
                                "border-top-color: rgb(70, 70, 70);"
                                "border-left-color:  rgb(70, 70, 70);"
                                "border-width : 1 2 2 1px;"
                                "border-style: solid;"
                                "border-radius: 10px;"
                                "}"
                                ".QPushButton, #numberLbl {"
                                "font-size: %1px;"
                                "font-weight: 900;"
                                "}"
                                "#titleLine {"
                                "background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1,"
                                "stop:0 rgba(255, 255, 255, 0),"
                                "stop:0.5 rgba(255,255,255,255),"
                                "stop:1 rgba(255,255,255,0));"
                                "border-style: none;"
                                "}"
                                "#titleLbl {"
                                "font-size: %2px;"
                                "color: white;"
                                "}"
                                "#posBar {"
                                "background-color: rgb(36,36,36);"
                                "border-color:  rgb(0,0,0);"
                                "border-top-color: rgb(70, 70, 70);"
                                "border-left-color:  rgb(70, 70, 70);"
                                "border-width : 1 2 2 1px;"
                                "border-style: solid;"
                                "}"
                                "#posBar::groove:horizontal {"
                                "border-radius: 0px;"
                                "}"
                                "#posBar::sub-page:horizontal {"
                                "background-color: rgb(0,80,255);"
                                "}"
                                "#posBar::add-page:horizontal {"
                                "}"
                                "#posBar::handle:horizontal {"
                                "background-color: rgb(36,36,36);"
                                "width: 1px;"
                                "}"
                                "#posLbl, #nonstopLbl {"
                                "font-size: %3px;"
                                "}"
                                "#nonstopIcon {"
                                "border: none;"
                                "background: transparent;"
                                "}")
                        .arg(qMax(1, numberBtns[0]->width()/3))
                        .arg(qMax(1, numberBtns[0]->width()/6))
                        .arg(qMax(1, numberBtns[0]->width()/6)));

    QSize iconSize = QSize(playBtn->width()/3, playBtn->width()/3);
    playBtn->setIconSize(iconSize);

    iconSize = QSize(btnBack->width()/2, btnBack->width()/2);
    btnBack->setIconSize(iconSize);
    rndBtn->setIconSize(iconSize);

    iconSize = QSize(numberBtns[0]->width()/6, numberBtns[0]->width()/6);
    nonstopIcon->setIconSize(iconSize);
    nonstopIcon->setFixedSize(iconSize);
}

void BigPanel::addDigit(int digit)
{
    if (player->isPlaying())
        return;

    int newNumber = (numberLbl->text() + QString::number(digit)).toInt();

    if (digit == 0 && newNumber == 0)
       return;

    int maxSong = -1;

    if(!player->songs.isEmpty())
        maxSong = player->songs.lastKey();

    if (newNumber <= maxSong)
    {
       numberLbl->setText(QString::number(newNumber));
       updateTitle(newNumber);
    }
    this->setNonstop(false);
}

void BigPanel::btnRnd_clicked()
{
    if (player->isPlaying())
        return;

    int random;
    if((numberLbl->text().length() > 0) && !nonstop)
        random = numberLbl->text().toInt();
    else
        random = rndPlaylist->getNext();
    numberLbl->setText(QString::number(random));
    updateTitle(random);
    this->setNonstop(true);
}

void BigPanel::numberBtn_clicked()
{
    this->addDigit(((MyPushButton*)QObject::sender())->text().toInt());
}

void BigPanel::btnBack_clicked()
{
    if (player->isPlaying())
        return;

    if (numberLbl->text().length() > 0)
       numberLbl->setText(numberLbl->text().left(numberLbl->text().length() - 1));

    if (numberLbl->text().length() > 0)
    {
       int newNumber = numberLbl->text().toInt();
       updateTitle(newNumber);
    }
    else
       updateTitle(0);
}

void BigPanel::updateTitle(int number)
{
    if(player->songs.contains(number))
    {
        titleLbl->setText(player->songs[number].title);
        numberLbl->setStyleSheet("");
        titleLine->setVisible(true);

        posBar->setWaveform(&player->songs[number].waveform);
        this->playerPositionChanged(-1, player->songs[number].duration/1000.0);
    }
    else
    {
        numberLbl->setStyleSheet("color: rgb(127, 127, 127);");
        titleLbl->setText("");
        titleLine->setVisible(false);

        posBar->setWaveform(nullptr);
        this->playerPositionChanged(-1, -1);
    }
}

void BigPanel::playBtn_clicked()
{
    if(player->isPlaying())
    {
        this->setNonstop(false);
        emit stop();
    }
    else
    {
        int number = numberLbl->text().toInt();
        if(number == 0) return;
        playBtn->setIcon(QIcon(":/player-img/stop.svg"));
        qApp->processEvents();
        emit play(number);
    }

    this->update();
}

void BigPanel::setNonstop(bool isSet)
{
    nonstop = isSet;

    nonstopIcon->setIcon(QIcon(isSet ? ":/player-img/nonstop-on.svg" : ":/player-img/nonstop-off.svg"));
    nonstopLbl->setStyleSheet(isSet ? "color: rgb(0, 80, 255);" : "color: rgb(30, 40, 45);");
}

void BigPanel::playerStateChanged(bool isPlaying)
{
    if(isPlaying)
    {
        playBtn->setIcon(QIcon(":/player-img/stop.svg"));
    }
    else
    {
        if(nonstop)
        {
            int random = rndPlaylist->getNext();
            numberLbl->setText(QString::number(random));
            this->updateTitle(random);
            emit play(random);
            isPlaying = true;
        }
        else
        {
            numberLbl->setText("");
            this->updateTitle(0);
            this->playerPositionChanged(-1, -1);

            playBtn->setIcon(QIcon(":/player-img/play.svg"));
        }

    }

    posBar->setEnabled(isPlaying);
}

void BigPanel::playerPositionChanged(double pos, double duration)
{
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

ShufflePlaylist::ShufflePlaylist(QMap<int, Song> *songs) : songs(songs)
{
    mtEngine = new std::mt19937(time(NULL));

    this->generateNewPlaylist();
}

ShufflePlaylist::~ShufflePlaylist()
{
    delete mtEngine;
}

int ShufflePlaylist::getNext()
{
    if(currPos >= playlist.size())
        generateNewPlaylist();

    if(playlist.size() < 1)
        return 0;

    return playlist[currPos++];
}

void ShufflePlaylist::generateNewPlaylist()
{
    currPos = 0;
    playlist.clear();

    auto numbers = songs->keys();
    for(int i=0; i<numbers.size(); i++)
    {
        playlist.push_back(numbers[i]);
    }

    shuffle(playlist);
}

void ShufflePlaylist::shuffle(QVector<int>& vec)
{
    std::uniform_int_distribution<int> distribution(0, vec.size()-1);
    for(int i=0; i<vec.size(); i++)
    {
        qSwap(vec[i], vec[distribution(*mtEngine)]);
    }
}

