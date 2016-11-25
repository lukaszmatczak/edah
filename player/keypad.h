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

#ifndef KEYPAD_H
#define KEYPAD_H

#include <libedah/mypushbutton.h>
#include <libedah/popup.h>

#include <QWidget>
#include <QFrame>
#include <QLabel>

class Player;

class Keypad : public Popup
{
    Q_OBJECT
public:
    explicit Keypad(int number, Player *player, bool play, QWidget *parent = 0);

protected:
    bool eventFilter(QObject* obj, QEvent* event);
    void keyReleaseEvent(QKeyEvent *e);
    void resizeEvent(QResizeEvent *event);

private:
    void addDigit(int digit);
    void updateTitle(int number);
    void recalcSizes(const QSize &size);

    Player *player;

    QLabel *numberLbl;
    QLabel *titleLbl;
    MyPushButton *btnBack;
    MyPushButton *cancelBtn;
    MyPushButton *okBtn;
    QList<MyPushButton*> numberBtns;

private slots:
    void btnBack_clicked();
    void numberBtn_clicked();
    void okBtn_clicked();

signals:
    void songEntered(int number);
};

#endif // KEYPAD_H
