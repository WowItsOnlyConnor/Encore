#include "MenuManager.h"

void MenuManager::SwitchScreen(Screens screen) {
    currentScreen = screen;
    onNewMenu = true;
}
