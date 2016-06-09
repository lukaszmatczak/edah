#ifndef RECORDER_H
#define RECORDER_H

#include <libedah/iplugin.h>

#include <QObject>
#include <QLabel>

class Recorder : public QObject, public IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "edah.iplugin" FILE "recorder.json")
    Q_INTERFACES(IPlugin)

public:
    Recorder(QObject *parent = 0);
    virtual ~Recorder();

    QWidget *getBigWidget();
    QWidget *getSmallWidget();
    QWidget *getSettingsTab();
    QString getPluginName() const;
    QString getPluginId() const;

    void loadSettings();
    void writeSettings();

private:
    QWidget *bigFrame;
    QWidget *smallWidget;
};

#endif // RECORDER_H
