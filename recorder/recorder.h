#ifndef RECORDER_H
#define RECORDER_H

#include <QObject>

#include <edah/iplugin.h>


class Recorder : public QObject, public IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "edah.iplugin" FILE "recorder.json")
    Q_INTERFACES(IPlugin)

public:
    Recorder(QObject *parent = 0);
    virtual ~Recorder();

    QWidget *getBigFrame();
    QWidget *getSettingsTab();
    QString getPluginName();

    void loadSettings();
    void writeSettings();

private:
    QWidget *bigFrame;
};

#endif // RECORDER_H
