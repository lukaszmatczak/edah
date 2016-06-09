#include "player.h"

#include <libedah/database.h>

#include <QWidget>
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>

#include <QDebug>

Player::Player()
{
    bigFrame = new QWidget;
    bigFrame->setLayout(new QGridLayout);

    QPushButton *btn = new QPushButton("Hello Player plugin!", bigFrame);
    bigFrame->layout()->addWidget(btn);

    settingsTab = new QWidget;

    smallWidget = new QLabel(this->getPluginName());
}

Player::~Player()
{
    delete bigFrame;
    delete smallWidget;
    delete settingsTab;
}

QWidget *Player::getBigWidget()
{
    return bigFrame;
}

QWidget *Player::getSmallWidget()
{
    return smallWidget;
}

QWidget *Player::getSettingsTab()
{
    return settingsTab;
}

QString Player::getPluginName() const
{
    return tr("Player");
}

QString Player::getPluginId() const
{
    return "player";
}

void Player::loadSettings()
{

}

void Player::writeSettings()
{

}

