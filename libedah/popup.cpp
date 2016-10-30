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

#include "popup.h"
#include "bluropacityeffect.h"
#include "logger.h"

#include <QTimeLine>
#include <QEventLoop>
#include <QResizeEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>
#include <QDebug>
Popup::Popup(QWidget *parent) :
    QDialog(nullptr), percentHeight(0.5f), percentWidth(0.5f), parent(parent)
{
    this->setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
    this->setModal(true);

    QFile fstyle(":/style.qss");
    if(!fstyle.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        LOG(QString("Couldn't open file \"%1\"").arg(fstyle.fileName()));
    }
    stylesheet = fstyle.readAll();

    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setAttribute(Qt::WA_AcceptTouchEvents);
    this->grabGesture(Qt::TapGesture);
}

void Popup::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e)

    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void Popup::closeEvent(QCloseEvent *e)
{
    Q_UNUSED(e)

    QTimeLine timeLine(200);
    BlurOpacityEffect *blurEffect = new BlurOpacityEffect(nullptr);
    blurEffect->setBlurRadiusAndOpacity(0.0f, 1.0f);
    this->setGraphicsEffect(blurEffect);

    timeLine.setFrameRange(0, 100);
    connect(&timeLine, &QTimeLine::frameChanged, this, [blurEffect](int frame) {
        blurEffect->setBlurRadiusAndOpacity(frame, (100-frame)/100.0f);
    });
    timeLine.start();

    QEventLoop loop;
    connect(&timeLine, &QTimeLine::finished, &loop, &QEventLoop::quit);
    loop.exec();
}

int Popup::exec()
{
    this->showAnimated();

    QEventLoop loop;
    loop.exec();

    return QDialog::Accepted;
}

void Popup::setStyleSheet(const QString &stylesheet)
{
    QDialog::setStyleSheet(this->stylesheet +
                           QString("\nPopup {"
                                   "   background-color: rgb(40, 40, 40);"
                                   "   border-color:  rgb(0,0,0);"
                                   "   border-top-color: rgb(70, 70, 70);"
                                   "   border-left-color:  rgb(70, 70, 70);"
                                   "   border-width : 2 4 4 2px;"
                                   "   border-style: solid;"
                                   "   border-radius: %1px;"
                                   "}\n")
                           .arg(qMax(1, this->height()/32)) +
                           stylesheet);
}

void Popup::showAnimated()
{
    this->resize();

    QTimeLine timeLine(200);
    BlurOpacityEffect *blurEffect = new BlurOpacityEffect(nullptr);
    blurEffect->setBlurRadiusAndOpacity(100.0f, 0.0f);
    this->setGraphicsEffect(blurEffect);
    this->show();

    timeLine.setFrameRange(100, 0);
    connect(&timeLine, &QTimeLine::frameChanged, this, [blurEffect](int frame) {
        blurEffect->setBlurRadiusAndOpacity(frame, (100-frame)/100.0f);
    });
    timeLine.start();

    QEventLoop loop;
    connect(&timeLine, &QTimeLine::finished, &loop, &QEventLoop::quit);
    loop.exec();

    this->setGraphicsEffect(nullptr);
}

void Popup::setSize(float width, float height)
{
    this->percentWidth = width;
    this->percentHeight = height;
}

void Popup::resize()
{
    const int width = parent->width()*percentWidth;
    const int height = parent->height()*percentHeight;

    this->setGeometry(parent->mapToGlobal(QPoint(0,0)).x() + (parent->width()-width)/2,
                      parent->mapToGlobal(QPoint(0,0)).y() + (parent->height()-height)/2,
                      width,
                      height);
}
