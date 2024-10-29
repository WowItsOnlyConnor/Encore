#pragma once
//
// Created by marie on 20/10/2024.
//

#include "menu.h"

#include <vector>

// technically this IS a menu, but realistically, is it?
class GameplayMenu : public Menu {
    int CameraSelectionPerPlayer[4][4] {
        {0,0,0,0},
        {1,0,0,0},
        {2,1,0,0},
        {3,2,1,0}
    };
    int CameraPosPerPlayer[4][4] {
        {0,0,0,0},
        {8,-8,0,0},
        {4,0,-4,0},
        {4,12,-12,-4}
    };
public:
    GameplayMenu();
    virtual ~GameplayMenu();
    virtual void Draw();
    virtual void Load();
};
