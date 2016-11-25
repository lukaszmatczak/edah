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

#include "keypad.h"
#include "player.h"

#include <QGridLayout>
#include <QPainter>
#include <QStyleOption>

#include <QDebug>

Keypad::Keypad(int number, Player *player, bool play, QWidget *parent) : Popup(parent), player(player)
{
    QGridLayout *layout = new QGridLayout(this);
    this->setLayout(layout);

    for(int i=0; i<6; i++)
        layout->setRowStretch(i, 3);

    layout->setRowStretch(0, 2);

    layout->setColumnStretch(0, 1);
    for(int i=1; i<4; i++)
        layout->setColumnStretch(i, 2);
    layout->setColumnStretch(4, 1);

    titleLbl = new QLabel(this);
    titleLbl->setObjectName("titleLbl");
    titleLbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
    titleLbl->setWordWrap(true);
    titleLbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    layout->addWidget(titleLbl, 0, 1, 1, 3);

    numberLbl = new QLabel(this);
    numberLbl->setObjectName("numberLbl");
    numberLbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
    numberLbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    layout->addWidget(numberLbl, 1, 1, 1, 2);

    btnBack = new MyPushButton("", this);
    btnBack->setIcon(QIcon(":/player-img/back.svg"));
    btnBack->setToolTip(tr("Delete last digit (<i>Backspace</i>)"));
    btnBack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    btnBack->setFocusPolicy(Qt::NoFocus);
    connect(btnBack, &MyPushButton::clicked, this, &Keypad::btnBack_clicked);
    layout->addWidget(btnBack, 1, 3);

    MyPushButton *btn0 = new MyPushButton("0", this);
    btn0->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    btn0->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(btn0, 5, 2);
    numberBtns.push_back(btn0);

    for(int i=0; i<9; i++)
    {
        MyPushButton *btn = new MyPushButton(QString::number(i+1), this);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        btn->setFocusPolicy(Qt::NoFocus);
        layout->addWidget(btn, i/3+2, i%3+1);
        numberBtns.push_back(btn);
    }

    for(int i=0; i<numberBtns.size(); i++)
    {
        connect(numberBtns[i], &MyPushButton::clicked, this, &Keypad::numberBtn_clicked);
    }

    cancelBtn = new MyPushButton("", this);
    cancelBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    cancelBtn->setFocusPolicy(Qt::NoFocus);
    cancelBtn->setIcon(QIcon(":/player-img/keypad-cancel.svg"));
    cancelBtn->setToolTip(tr("Cancel (<i>Esc</i>)"));
    connect(cancelBtn, &MyPushButton::pressed, this, &Keypad::close);
    layout->addWidget(cancelBtn, 5, 1);

    okBtn = new MyPushButton("", this);
    okBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    if(play)
    {
        okBtn->setIcon(QIcon(":/player-img/keypad-play.svg"));
        okBtn->setToolTip(tr("Play (<i>Enter</i>)"));
    }
    else
    {
        okBtn->setIcon(QIcon(":/player-img/keypad-ok.svg"));
        okBtn->setToolTip(tr("OK (<i>Enter</i>)"));
    }
    connect(okBtn, &MyPushButton::pressed, this, &Keypad::okBtn_clicked);
    layout->addWidget(okBtn, 5, 3);

    this->updateTitle(-1);

    if(number > 0 && number < 10)
        addDigit(number);

    this->installEventFilter(this);
}

bool Keypad::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* key = static_cast<QKeyEvent*>(event);
        if ((key->key() == Qt::Key_Enter) || (key->key() == Qt::Key_Return))
        {
            this->okBtn_clicked();
            return true;
        }
        else
        {
            return QObject::eventFilter(obj, event);
        }
    }
    else
    {
        return QObject::eventFilter(obj, event);
    }
}

void Keypad::keyReleaseEvent(QKeyEvent *e)
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
    else if(e->key() == Qt::Key_Escape)
    {
        this->close();
        e->setAccepted(true);
    }
}

void Keypad::resizeEvent(QResizeEvent *event)
{
    this->recalcSizes(event->size());
}

void Keypad::recalcSizes(const QSize &size)
{
    Popup::setStyleSheet(QString(".QPushButton, #numberLbl {"
                                "   font-size: %1px;"
                                "   font-weight: 900;"
                                "}"
                                "#titleLbl {"
                                "   font-size: %2px;"
                                "   color: white;"
                                "}")
                        .arg(qMax(1, size.height()/12))
                        .arg(qMax(1, size.height()/26)));

    QSize iconSize = QSize(size.height()/12, size.height()/12);
    btnBack->setIconSize(iconSize);
    okBtn->setIconSize(iconSize);
    cancelBtn->setIconSize(iconSize);
}

void Keypad::addDigit(int digit)
{
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
}

void Keypad::updateTitle(int number)
{
    if(player->songs.contains(number))
    {
        titleLbl->setText(player->songs[number].title);
        numberLbl->setStyleSheet("");
    }
    else
    {
        numberLbl->setStyleSheet("color: rgb(127, 127, 127);");
        titleLbl->setText(tr("Enter song number"));
    }
}

void Keypad::numberBtn_clicked()
{
    this->addDigit(((MyPushButton*)QObject::sender())->text().toInt());
}

void Keypad::btnBack_clicked()
{
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

void Keypad::okBtn_clicked()
{
    if(!numberLbl->text().isEmpty())
    {
        emit songEntered(numberLbl->text().toInt());
        this->close();
    }
}
