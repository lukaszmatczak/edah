#include "recorder.h"

#include <QWidget>
#include <QPushButton>
#include <QGridLayout>

#include <QDebug>

Recorder::Recorder(QObject *parent) :
    QObject(parent)
{
    bigFrame = new QWidget;
    bigFrame->setLayout(new QGridLayout);

    QPushButton *btn = new QPushButton("Hello Recorder plugin!", bigFrame);
    bigFrame->layout()->addWidget(btn);

    smallWidget = new QLabel(this->getPluginName());
}

Recorder::~Recorder()
{
    delete bigFrame;
}

QWidget *Recorder::getBigWidget()
{
    return bigFrame;
}

QWidget *Recorder::getSmallWidget()
{
    return smallWidget;
}

QWidget *Recorder::getSettingsTab()
{
    return nullptr;
}

QString Recorder::getPluginName()
{
    return tr("Recorder");
}

void Recorder::loadSettings()
{

}

void Recorder::writeSettings()
{

}
