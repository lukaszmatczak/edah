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

BigPanel::BigPanel(Player *parent) : QWidget(0), parent(parent)
{
    layout = new QGridLayout(this);
    this->setLayout(layout);

    MyPushButton *btn0 = new MyPushButton(QString::number(0), this);
    btn0->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(btn0, 3, 1);
    numberBtns.push_back(btn0);

    MyPushButton *btnBack = new MyPushButton("<-", this);
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

    titleLbl = new QLabel(this);
    titleLbl->setObjectName("titleLbl");
    titleLbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
    titleLbl->setWordWrap(true);
    titleLbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    songInfoFrm->layout()->addWidget(titleLbl);

    for(int i=0; i<layout->columnCount(); i++)
    {
        layout->setColumnStretch(i, 1);
    }

    for(int i=0; i<layout->rowCount(); i++)
    {
        layout->setRowStretch(i, 1);
    }
}

void BigPanel::resizeEvent(QResizeEvent *e)
{
    layout->setSpacing((e->size().width()+e->size().height())/128);

    QFontMetrics fm = QFontMetrics(numberLbl->font());
    numberLbl->setFixedWidth(fm.width("000"));

    this->setStyleSheet(QString("#songInfoFrm { background-color: rgb(40,40,40);"
                                "border-color:  rgb(0,0,0);"
                                "border-top-color: rgb(70, 70, 70);"
                                "border-left-color:  rgb(70, 70, 70);"
                                "border-width : 2 4 4 2px;"
                                "border-style: solid;"
                                "border-radius: 10px;"
                                "}"
                                ".QPushButton, #numberLbl {"
                                "font-size: %1px;"
                                "font-weight: 900;"
                                "}"
                                "#titleLbl {"
                                "font-size: %2px;"
                                "color: white;"
                                "}")
                        .arg(numberBtns[0]->width()/3)
                        .arg(numberBtns[0]->width()/6));

}

void BigPanel::numberBtn_clicked()
{
    //if (playRecAudio->isPlaying())
    //    return;

    QString digit = ((MyPushButton*)QObject::sender())->text();

    int newNumber = (numberLbl->text() + digit).toInt();

    if (digit == "0" && newNumber == 0)
       return;

    int maxSong = -1;

    if(!parent->songs.isEmpty())
        maxSong = parent->songs.lastKey();

    if (newNumber <= maxSong)
    {
       numberLbl->setText(QString::number(newNumber));
       updateTitle(newNumber);
    }
    //setNonstop(false);
}

void BigPanel::btnBack_clicked()
{
    //if (playRecAudio->isPlaying())
    //    return;


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
    if(number > 0)
    {
        titleLbl->setText(parent->songs[number].title);
        //ui->playline->setVisible(true);
    }
    else
    {
        titleLbl->setText("");
        //ui->playline->setVisible(false);
    }
}
