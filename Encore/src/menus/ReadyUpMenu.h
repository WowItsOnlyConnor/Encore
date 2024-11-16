//
// Created by marie on 16/11/2024.
//

#ifndef READYUPMENU_H
#define READYUPMENU_H
#include "OvershellMenu.h"

class ReadyUpMenu : public OvershellMenu {
public:
    ReadyUpMenu() = default;
    ~ReadyUpMenu() override = default;
    void Draw() override;
    void Load() override;
};



#endif //READYUPMENU_H
