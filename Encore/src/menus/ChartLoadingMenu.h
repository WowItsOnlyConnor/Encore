//
// Created by marie on 17/11/2024.
//

#ifndef CHARTLOADINGMENU_H
#define CHARTLOADINGMENU_H
#include "OvershellMenu.h"

class ChartLoadingMenu : public OvershellMenu {
public:
    ChartLoadingMenu() {};
    ~ChartLoadingMenu() {};
    void Draw() override;
    void Load() override;
};
#endif //CHARTLOADINGMENU_H
