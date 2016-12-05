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

#include "windowselector.h"
#include "playlistmodel.h"
#include "player.h"

#include <libedah/utils.h>
#include <libedah/mypushbutton.h>

#include <QFrame>
#include <QGuiApplication>
#include <QScreen>
#include <QGraphicsDropShadowEffect>
#include <QSequentialAnimationGroup>
#include <QPropertyAnimation>
#include <QCheckBox>
#include <QPushButton>
#include <QGestureEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>

bool WindowSelector::scaleChkState = false;
bool WindowSelector::mouseChkState = false;

WindowSelector::WindowSelector(QWidget *mainWindow, QWidget *videoWindow, QWidget *parent) :
    QWidget(parent), mainWindow(mainWindow), videoWindow(videoWindow), selected(false), prevThumb(nullptr)
{
    this->setStyleSheet(QString("\
QWidget { \
    background: transparent; \
    color: white; \
    font: %1px \"Open Sans Light\"; \
} \
QPushButton { \
    border-color: rgb(0,0,0); \
    border-top-color: rgb(70, 70, 70); \
    border-left-color: rgb(70, 70, 70); \
    border-width : 1 2 2 1px; \
    border-style: solid; \
    border-radius: 10px; \
    background-color: rgb(36,36,36); \
} \
QPushButton:pressed { \
    border-color: rgb(0,0,0); \
    border-bottom-color: rgb(70, 70, 70); \
    border-right-color: rgb(70, 70, 70); \
    border-width: 2 1 1 2px; \
} \
QCheckBox::indicator { \
    width: %2px; \
    height: %2px; \
} \
QCheckBox::indicator:checked         { image: url(:/player-img/checkbox_checked.svg); } \
QCheckBox::indicator:checked:hover   { image: url(:/player-img/checkbox_checked_hover.svg); } \
QCheckBox::indicator:unchecked       { image: url(:/player-img/checkbox_unchecked.svg); } \
QCheckBox::indicator:unchecked:hover { image: url(:/player-img/checkbox_unchecked_hover.svg); } \
QCheckBox::indicator:disabled        { image: url(:/player-img/checkbox_disabled.svg); } \
QCheckBox:disabled                   { color: rgb(127,127,127); }")
                                .arg(mainWindow->height()/29)
                                .arg(mainWindow->height()/34));
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setAttribute(Qt::WA_AcceptTouchEvents);
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::ToolTip);
    this->setWindowTitle("Edah_WindowSelector");
    this->setCursor(Qt::CrossCursor);

    areaFrm = new QFrame(this);
    areaFrm->setStyleSheet("background-color: rgba(127,127,127,2)");
    areaFrm->move(0, 0);
    areaFrm->show();

    QRect geom = QGuiApplication::primaryScreen()->geometry();
    this->setGeometry(geom);
    areaFrm->resize(geom.size());

    borderFrm = new QFrame(this);
    borderFrm->setStyleSheet("background-color: rgba(127,127,255,64); border: 4px solid #8080ff;");
    borderFrm->setGeometry(0,0,0,0);

    infoLbl = new QLabel(tr("Select window to clone\ninto secondary screen.\n\nPress ESC to cancel."), this);
    infoLbl->setObjectName("infoLbl");
    infoLbl->resize(mainWindow->height()/1.5f, mainWindow->height()/4);
    infoLbl->move((geom.width()-infoLbl->width())/2, (geom.height()-infoLbl->height())/2);
    infoLbl->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    infoLbl->setStyleSheet("#infoLbl { background-color: rgba(20,20,20,200); border: 2px solid black; border-radius: 10px; }");

    QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(infoLbl);
    effect->setBlurRadius(30);
    effect->setColor(Qt::black);
    effect->setOffset(0,0);
    infoLbl->setGraphicsEffect(effect);

    infoLbl->show();

    this->setMouseTracking(true);
    areaFrm->setMouseTracking(true);
    borderFrm->setMouseTracking(true);
    infoLbl->setMouseTracking(true);

    connect(&updateTimer, &QTimer::timeout, this, &WindowSelector::update);
    updateTimer.start(1000/30);
    hoverWindow = 0;

    this->grabKeyboard();
}

WindowSelector::~WindowSelector()
{
    if(this->prevThumb)
        delete prevThumb;

    this->releaseKeyboard();
}

void WindowSelector::update()
{
    if(hoverWindow)
        borderFrm->setGeometry(Player::getWindowRect(hoverWindow).marginsAdded(QMargins(2,2,2,2)));
}

void WindowSelector::mouseMoveEvent(QMouseEvent *e)
{
    if(!selected)
    {
        QList<WId> skipWindows;
        skipWindows << this->winId() << mainWindow->winId();
        WindowInfo wi = Player::getWindowAt(e->pos(), skipWindows);

        if(wi.windowID == videoWindow->winId())
        {
            borderFrm->setStyleSheet("background-color: rgba(127,127,127,64);"
                                     "border: 4px solid #808080;");
        }
        else
        {
            borderFrm->setStyleSheet("background-color: rgba(127,127,255,64);"
                                     "border: 4px solid #8080ff;");
        }

        borderFrm->setGeometry(wi.geometry.marginsAdded(QMargins(2,2,2,2)));
        hoverWindow = wi.windowID;
    }
}

void WindowSelector::mousePressEvent(QMouseEvent *e)
{
    if(e->button() == Qt::LeftButton && !this->selected)
    {
        QList<WId> skipWindows;
        skipWindows << this->winId() << mainWindow->winId();
        WindowInfo wi = Player::getWindowAt(e->pos(), skipWindows);

        if((wi.windowID == mainWindow->winId()) || (wi.windowID == videoWindow->winId()))
            return;

        this->selected = true;

        QSequentialAnimationGroup *animGroup = new QSequentialAnimationGroup(this);
        const int destWidth = mainWindow->height()/0.95f, destHeight = mainWindow->height()/1.34f;

        QPropertyAnimation *widthAnim = new QPropertyAnimation(infoLbl, "geometry", this);
        widthAnim->setDuration((destWidth-infoLbl->width())*2);
        widthAnim->setStartValue(infoLbl->geometry());
        widthAnim->setEndValue(QRect(infoLbl->x()-(destWidth-infoLbl->width())/2,
                                     infoLbl->y(),
                                     destWidth, infoLbl->height()));
        widthAnim->setEasingCurve(QEasingCurve::InOutCubic);
        animGroup->addAnimation(widthAnim);

        QPropertyAnimation *heightAnim = new QPropertyAnimation(infoLbl, "geometry", this);
        heightAnim->setDuration((destHeight-infoLbl->height())*2);
        heightAnim->setStartValue(widthAnim->endValue());
        heightAnim->setEndValue(QRect(infoLbl->x()-(destWidth-infoLbl->width())/2,
                                      infoLbl->y()-(destHeight-infoLbl->height())/2,
                                      destWidth, destHeight));
        heightAnim->setEasingCurve(QEasingCurve::InOutCubic);
        animGroup->addAnimation(heightAnim);

        animGroup->start();

        QEventLoop loop;
        connect(animGroup, &QSequentialAnimationGroup::finished, &loop, &QEventLoop::quit);
        loop.exec();

        infoLbl->setText("");

        QVBoxLayout *layout = new QVBoxLayout;
        infoLbl->setLayout(layout);

        QLabel *previewBg = new QLabel(infoLbl);
        previewBg->resize(videoWindow->size().scaled(destWidth-20, mainWindow->height()/2.35f, Qt::KeepAspectRatio));
        previewBg->move((destWidth-previewBg->width())/2, 15);
        previewBg->setStyleSheet("background-color: black;");
        previewBg->setAlignment(Qt::AlignCenter | Qt::AlignHCenter);
        previewBg->show();
        layout->addSpacing(previewBg->height()+10);

        if(prevThumb)
            delete prevThumb;

        prevThumb = new WindowThumbnail(wi.windowID, previewBg, true, false, false);
        float thumbScale = qMin((float)previewBg->width()/videoWindow->width(),
                                (float)previewBg->height()/videoWindow->height());
        prevThumb->setScale(thumbScale);
        prevThumb->move(QSize());

        QCheckBox *scaleChk = new QCheckBox(tr("Scale preview of the window to screen size"), infoLbl);
        connect(scaleChk, &QCheckBox::stateChanged, this, [this, previewBg](int state) {
            float thumbScale = -1.0f;
            if(state == Qt::Unchecked)
            {
                thumbScale = qMin((float)previewBg->width()/videoWindow->width(),
                                  (float)previewBg->height()/videoWindow->height());
            }

            prevThumb->setScale(thumbScale);
            prevThumb->move(QSize());
        });
        scaleChk->setChecked(this->scaleChkState);
        layout->addWidget(scaleChk);

        QCheckBox *adjustChk = new QCheckBox(tr("Adjust window size to screen resolution (%1x%2)")
                                             .arg(videoWindow->width())
                                             .arg(videoWindow->height()),
                                             infoLbl);
        const QRect screenGeom = QGuiApplication::primaryScreen()->availableGeometry();
        if(videoWindow->width() > screenGeom.width() || videoWindow->height() > screenGeom.height())
            adjustChk->setEnabled(false);

        connect(adjustChk, &QCheckBox::stateChanged, this, [this, wi](int state) {
            static QSize oldSize = Player::getWindowRect(wi.windowID).size();

            if(state == Qt::Checked)
                Player::setWindowSize(wi.windowID, videoWindow->size());
            else
                Player::setWindowSize(wi.windowID, oldSize);

            borderFrm->setGeometry(Player::getWindowRect(wi.windowID).marginsAdded(QMargins(2,2,2,2)));
            hoverWindow = wi.windowID;
        });
        layout->addWidget(adjustChk);

        QCheckBox *mouseChk = new QCheckBox(tr("Show mouse cursor"), infoLbl);
        mouseChk->setChecked(this->mouseChkState);
        layout->addWidget(mouseChk);

        QHBoxLayout *btnLayout = new QHBoxLayout;
        btnLayout->addStretch(2);
        layout->addLayout(btnLayout);

        MyPushButton *okBtn = new MyPushButton(tr("OK"), infoLbl);
        okBtn->setMinimumHeight(mainWindow->height()/15);
        connect(okBtn, &MyPushButton::clicked, this, [this, scaleChk, adjustChk, mouseChk, wi]() {
            this->scaleChkState = scaleChk->isChecked();
            this->mouseChkState = mouseChk->isChecked();

            int flags = 0;
            flags |= scaleChk->isChecked() * EF_WIN_SCALE;
            flags |= mouseChk->isChecked() * EF_WIN_WITHCURSOR;
            emit windowSelected(wi.windowID, flags);
        });
        btnLayout->addWidget(okBtn, 1);

        MyPushButton *cancelBtn = new MyPushButton(tr("Cancel"), infoLbl);
        cancelBtn->setMinimumHeight(mainWindow->height()/15);
        connect(cancelBtn, &MyPushButton::clicked, this, &WindowSelector::close);
        btnLayout->addWidget(cancelBtn, 1);

        this->setCursor(Qt::ArrowCursor);
    }
}

void WindowSelector::keyReleaseEvent(QKeyEvent *e)
{
    if(e->key() == Qt::Key_Escape)
        this->close();
}

bool WindowSelector::close()
{
    emit closeSignal();
    return QWidget::close();
}
