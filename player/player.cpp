#include "player.h"

#include <QWidget>
#include <QPushButton>
#include <QGridLayout>

Player::Player(QObject *parent) :
    QObject(parent)
{
    bigFrame = new QWidget;
    bigFrame->setLayout(new QGridLayout);

    QPushButton *btn = new QPushButton("Hello Player plugin!", bigFrame);
    bigFrame->layout()->addWidget(btn);
}

Player::~Player()
{
    delete bigFrame;
}

QWidget *Player::getBigFrame()
{
    return bigFrame;
}
