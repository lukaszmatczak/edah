#ifndef PLAYER_H
#define PLAYER_H

#include <QObject>

#include <edah/iplugin.h>

class Player : public QObject, public IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "edah.iplugin" FILE "player.json")
    Q_INTERFACES(IPlugin)

public:
    Player(QObject *parent = 0);
    virtual ~Player();

    QWidget *getBigFrame();
    QWidget *getSettingsTab();
    QString getPluginName();

    void loadSettings();
    void writeSettings();

private:
    QWidget *bigFrame;
    QWidget *settingsTab;
};

#endif // PLAYER_H
