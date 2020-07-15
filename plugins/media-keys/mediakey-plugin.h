#ifndef MEDIAKEYPLUGIN_H
#define MEDIAKEYPLUGIN_H
#include "plugin-interface.h"
#include <QtCore/QtGlobal>
#include "mediakeysmanager.h"

class MediakeyPlugin : public PluginInterface
{
public:
    ~MediakeyPlugin();
    static PluginInterface* getInstance();

    virtual void activate () override;
    virtual void deactivate () override;

private:
    MediakeyPlugin();
    MediakeyPlugin(MediakeyPlugin&)=delete;

private:
    MediaKeysManager*        mManager;
    static PluginInterface*         mInstance;
};

extern "C" Q_DECL_EXPORT PluginInterface* createSettingsPlugin();

#endif // MEDIAKEYPLUGIN_H
