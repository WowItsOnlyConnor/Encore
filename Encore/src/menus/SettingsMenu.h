//
// Created by marie on 17/11/2024.
//

#ifndef SETTINGSMENU_H
#define SETTINGSMENU_H
#include "menu.h"
#include "settings.h"

class SettingsMenu : public Menu {
#define OPTION(type, value, default) type value = default;
    SETTINGS_OPTIONS;
#undef OPTION
public:
    SettingsMenu() {};
    ~SettingsMenu() {};
    void Load() override;
    void Draw() override;
};



#endif //SETTINGSMENU_H
