#include "player.h"

#include <QWidget>
#include <QPushButton>
#include <QGridLayout>

#include <QDebug>

Player::Player(QObject *parent) :
    QObject(parent)
{
    bigFrame = new QWidget;
    bigFrame->setLayout(new QGridLayout);

    QPushButton *btn = new QPushButton("Hello Player plugin!", bigFrame);
    bigFrame->layout()->addWidget(btn);

    settingsTab = new QWidget;
}

Player::~Player()
{
    delete bigFrame;
}

QWidget *Player::getBigFrame()
{
    return bigFrame;
}

QWidget *Player::getSettingsTab()
{
    return settingsTab;
}

QString Player::getPluginName()
{
    return tr("Player");
}

void Player::loadSettings()
{

}

void Player::writeSettings()
{

}

