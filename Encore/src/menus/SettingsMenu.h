//
// Created by marie on 17/11/2024.
//

#ifndef SETTINGSMENU_H
#define SETTINGSMENU_H
#include "menu.h"

class SettingsMenu : public Menu {
public:
    SettingsMenu() {};
    ~SettingsMenu() {};
    void Load() override {};
    void Draw() override;
};



#endif //SETTINGSMENU_H
