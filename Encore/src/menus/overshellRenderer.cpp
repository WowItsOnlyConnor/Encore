//
// Created by marie on 03/08/2024.
//

#include "overshellRenderer.h"
#include "raylib.h"
#include "raygui.h"
#include "uiUnits.h"
#include "assets.h"
#include "gameMenu.h"
#include "styles.h"
#include "users/playerManager.h"

/// std::vector<bool> SlotSelectingState = { false, false, false, false };
/// std::vector<bool> OpenState = { false, false, false, false };
/// std::vector<bool> InstrumentTypeState = { false, false, false, false };


/*


void DrawBeacon(int slot, float x, float y, float width, float height, bool top) {
    PlayerManager &playerManager = ThePlayerManager;
    Color overshellBeacon =
        ColorBrightness(playerManager.GetActivePlayer(slot)->AccentColor, -0.75f);
    Color thanksraylib = { overshellBeacon.r, overshellBeacon.g, overshellBeacon.b, 128 };
    float HalfWidth = width / 2;
    BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);
    for (int g = 0; g < 4; g++) {
        DrawRectangleGradientH(x, y, HalfWidth, height, { 0, 0, 0, 0 }, thanksraylib);
        DrawRectangleGradientH(
            x + HalfWidth - 1, y, HalfWidth, height, thanksraylib, { 0, 0, 0, 0 }
        );
    }
    EndBlendMode();
    Color BaseWitAllAlpha = ColorBrightness(GetColor(0x181827FF), -0.25f);
    Color BaseWitNoAlpha = { BaseWitAllAlpha.r, BaseWitAllAlpha.g, BaseWitAllAlpha.b, 0 };
    if (top) {
        DrawRectangleGradientV(x, y, width, height, BaseWitAllAlpha, BaseWitNoAlpha);
    }
}

bool DrawOvershellRectangleHeader(
    float x, float y, float width, float height, std::string username, Color accentColor, Color usernameColor
) {
    Assets &assets = Assets::getInstance();
    Units &unit = Units::getInstance();
    Rectangle RectPos = { x, y, width, height * 2 };
    bool toReturn = GuiButton({ x, y, width, height }, "");
    // float Inset = unit.winpct(0.001f);
    // float InsetDouble = Inset * 2;
    // DrawRectangleRounded(
    //     {RectPos.x + Inset, RectPos.y + Inset, RectPos.width - (InsetDouble*1.25f),
    //     RectPos.height - InsetDouble}, 0.40f, 5, ColorBrightness(accentColor, -0.75f)
    //);
    BeginScissorMode(x, y, width + 2, height);
    DrawRectangleRounded(RectPos, 0.40f, 8, ColorBrightness(accentColor, -0.5f));
    EndScissorMode();

    float centerPos = x + (width / 2);
    GameMenu::mhDrawText(
        assets.redHatDisplayBlack,
        username.c_str(),
        { centerPos, (height / 4) + y },
        (height / 2),
        usernameColor,
        assets.sdfShader,
        CENTER
    );
    return toReturn;
}

void OvershellRenderer::SetSlotState(int slot, int state) {

}

void OvershellRenderer::DrawTopOvershell(double height) {
    BeginBlendMode(BLEND_ALPHA);
    Units &unit = Units::getInstance();
    PlayerManager &playerManager = ThePlayerManager;
    DrawRectangleGradientV(
        0,
        unit.hpct(height) - 2,
        GetScreenWidth(),
        unit.hinpct(0.025f),
        Color { 0, 0, 0, 128 },
        Color { 0, 0, 0, 0 }
    );
    DrawRectangle(0, 0, (int)GetScreenWidth(), unit.hpct(height), WHITE);
    DrawRectangle(
        0,
        0,
        (int)GetScreenWidth(),
        unit.hpct(height) - unit.hinpct(0.005f),
        ColorBrightness(GetColor(0x181827FF), -0.25f)
    );

    for (int i = 0; i < 4; i++) {
        float OvershellTopLoc = unit.hpct(1.0f) - unit.winpct(0.05f);
        float OvershellLeftLoc =
            (unit.wpct(0.125) + (unit.winpct(0.25) * i)) - unit.winpct(0.1);
        float OvershellCenterLoc = (unit.wpct(0.125) + (unit.winpct(0.25) * i));
        float HalfWidth = OvershellCenterLoc - OvershellLeftLoc;
        if (playerManager.ActivePlayers[i] != -1) {
            DrawBeacon(
                i,
                OvershellLeftLoc,
                0,
                HalfWidth * 2,
                unit.hpct(height) - unit.hinpct(0.005f),
                true
            );
        }
    }
}

/*
 *this is code for the fuckin uh. it the shape of the menu.
if (GuiButton(
                            { OvershellLeftLoc,
                              unit.hpct(1.0f) - (unit.winpct(0.03f) * (x + 2)),
                              unit.winpct(0.2f),
                              unit.winpct(0.03f) },
                            playerManager.PlayerList[x].Name.c_str()
                        )) {
    playerManager.AddActivePlayer(x, i);
    CanMouseClick = true;
    SlotSelectingState[i] = false;
    gameMenu.shouldBreak = true;
                        }

bool MenuButton(int slot, int x, std::string string) {
    Units &u = Units::getInstance();
    float OvershellLeftLoc =
        (u.wpct(0.125) + (u.winpct(0.25) * slot)) - u.winpct(0.1);
    return GuiButton(
        { OvershellLeftLoc,
          u.hpct(1.0f) - (u.winpct(0.03f) * (x + 1)),
          u.winpct(0.2f),
          u.winpct(0.03f) },
        string.c_str()
    );
    SETDEFAULTSTYLE();
}
bool OvershellSlider(
    int slot, int x, std::string string, float *value, float step, float min, float max
) {
    Units &unit = Units::getInstance();
    float OvershellLeftLoc =
        (unit.wpct(0.125) + (unit.winpct(0.25) * slot)) - unit.winpct(0.1);
    float height = unit.winpct(0.03f);
    float widthNoHeight = unit.winpct(0.2f) - height;
    Rectangle bounds = { OvershellLeftLoc + height,
                         unit.hpct(1.0f) - (unit.winpct(0.03f) * (x + 1)),
                         unit.winpct(0.2f) - height - height,
                         height };
    Rectangle confirmBounds = { OvershellLeftLoc + widthNoHeight,
                                unit.hpct(1.0f) - (unit.winpct(0.03f) * (x + 1)),
                                height,
                                height };
    Assets &assets = Assets::getInstance();

    GuiSlider(bounds, "", "", value, min, max);
    GuiButton(
        { OvershellLeftLoc,
          unit.hpct(1.0f) - (unit.winpct(0.03f) * (x + 1)),
          height,
          height },
        TextFormat("%1.1f", *value)
    );
    *value = (round(*value / step) * step);

    if (GuiButton(confirmBounds, "<")) {
        return true;
    };
    return false;
}

bool OvershellCheckbox(int slot, int x, std::string string, bool initialVal) {
    Units &unit = Units::getInstance();
    float OvershellLeftLoc =
        (unit.wpct(0.125) + (unit.winpct(0.25) * slot)) - unit.winpct(0.1);
    float height = unit.winpct(0.03f);
    float widthNoHeight = unit.winpct(0.2f);
    Rectangle bounds = { OvershellLeftLoc + height,
                         unit.hpct(1.0f) - (unit.winpct(0.03f) * (x + 1)),
                         unit.winpct(0.2f) - height - height,
                         height };
    Rectangle confirmBounds = { OvershellLeftLoc + widthNoHeight,
                                unit.hpct(1.0f) - (unit.winpct(0.03f) * (x + 1)),
                                height,
                                height };
    Assets &assets = Assets::getInstance();

    if (GuiButton(
            { OvershellLeftLoc,
              unit.hpct(1.0f) - (unit.winpct(0.03f) * (x + 1)),
              widthNoHeight,
              height },
            string.c_str()
        )) {
        initialVal = !initialVal;
    }

    DrawRectanglePro(confirmBounds, { 0 }, 0, initialVal ? GREEN : RED);
    return initialVal;
}

bool BNSetting = false;
void OvershellRenderer::DrawBottomOvershell() {
    Assets &assets = Assets::getInstance();
    Units &unit = Units::getInstance();
    MainMenu &gameMenu = TheGameMenu;
    float BottomBottomOvershell = GetScreenHeight() - unit.hpct(0.1f);
    float InnerBottom = BottomBottomOvershell + unit.hinpct(0.005f);
    DrawRectangle(
        0, BottomBottomOvershell, (float)GetScreenWidth(), (float)GetScreenHeight(), WHITE
    );
    DrawRectangle(
        0,
        InnerBottom,
        (float)GetScreenWidth(),
        (float)GetScreenHeight(),
        ColorBrightness(GetColor(0x181827FF), -0.5f)
    );
    int ButtonHeight = unit.winpct(0.03f);
    PlayerManager &playerManager = ThePlayerManager;
    float LeftMin = unit.wpct(0.1);
    float LeftMax = unit.wpct(0.9);
    for (int i = 0; i < 4; i++) {
        bool EmptySlot = true;

        float OvershellTopLoc = unit.hpct(1.0f) - unit.winpct(0.05f);
        float OvershellLeftLoc =
            (unit.wpct(0.125) + (unit.winpct(0.25) * i)) - unit.winpct(0.1);
        float OvershellCenterLoc = (unit.wpct(0.125) + (unit.winpct(0.25) * i));
        float HalfWidth = OvershellCenterLoc - OvershellLeftLoc;
        switch (OvershellState[i]) {
        case CREATION: {
            static char name[32] = { 0 };
            Rectangle textBoxPosition { OvershellLeftLoc,
                                        unit.hpct(1.0f) - (unit.winpct(0.03f) * 3),
                                        unit.winpct(0.2f),
                                        unit.winpct(0.03f) };
            GuiTextBox(textBoxPosition, name, 32, true);
            if (MenuButton(i, 1, "Confirm")) {
                playerManager.CreatePlayer(name);
                playerManager.AddActivePlayer(playerManager.PlayerList.size() - 1, i);
                CanMouseClick = true;
                OvershellState[i] = OS_ATTRACT;
                continue;
            }
            if (MenuButton(i, 0, "Cancel")) {
                OvershellState[i] = CREATION;
                continue;
            }
            break;
        }
        case OS_PLAYER_SELECTION: {
            // for selecting players
            float InfoLoc = (playerManager.PlayerList.size() + 2);
            DrawOvershellRectangleHeader(
                OvershellLeftLoc,
                OvershellTopLoc - (ButtonHeight * InfoLoc),
                unit.winpct(0.2f),
                unit.winpct(0.05f),
                "Select a player",
                Color { 255, 0, 255, 255 },
                WHITE
            );
            if (GuiButton(
                    { OvershellLeftLoc,
                      unit.hpct(1.0f) - (unit.winpct(0.03f)),
                      unit.winpct(0.2f),
                      unit.winpct(0.03f) },
                    "Cancel"
                )) {
                CanMouseClick = true;
                OvershellState[i] = OS_ATTRACT;
                gameMenu.shouldBreak = true;
            }
            BeginBlendMode(BLEND_MULTIPLIED);
            DrawRectangleRec(
                { OvershellLeftLoc,
                  unit.hpct(1.0f) - (unit.winpct(0.03f)),
                  unit.winpct(0.2f),
                  unit.winpct(0.03f) },
                Color { 255, 0, 0, 128 }
            );
            EndBlendMode();
            if (!playerManager.PlayerList.empty()) {
                for (int x = 0; x < playerManager.PlayerList.size(); x++) {
                    if (playerManager.ActivePlayers[i] == -1) {
                        if (MenuButton(
                                i, x + 1, playerManager.PlayerList[x].Name.c_str()
                            )) {
                            playerManager.AddActivePlayer(x, i);
                            CanMouseClick = true;
                            OvershellState[i] = OS_ATTRACT;
                        }
                    }
                }
            }
            if (MenuButton(i, playerManager.PlayerList.size() + 1, "New Profile")) {
                OvershellState[i] = CREATION;
            }
            break;
        }
        case OS_OPTIONS: {
            Color headerUsernameColor = playerManager.GetActivePlayer(i)->Bot ? SKYBLUE : WHITE;
            if (DrawOvershellRectangleHeader(
                    OvershellLeftLoc,
                    OvershellTopLoc - (ButtonHeight * 5),
                    unit.winpct(0.2f),
                    unit.winpct(0.05f),
                    playerManager.GetActivePlayer(i)->Name,
                    playerManager.GetActivePlayer(i)->AccentColor,
                    headerUsernameColor
                )) {
                OvershellState[i] = OS_ATTRACT;
                CanMouseClick = true;
                continue;
            }
            if (!BNSetting) {
                if (MenuButton(i, 3, "Breakneck Speed")) {
                    BNSetting = true;
                    continue;
                }
            }
            if (BNSetting) {
                if (OvershellSlider(
                        i,
                        3,
                        "Breakneck Speed",
                        &playerManager.GetActivePlayer(i)->NoteSpeed,
                        0.25,
                        0.25,
                        3
                    )) {
                    BNSetting = false;
                    continue;
                };
            }
            if (MenuButton(i, 4, "Instrument Type")) {
                OvershellState[i] = OS_INSTRUMENT_SELECTIONS;
                break;
            }
            playerManager.GetActivePlayer(i)->Bot =
                OvershellCheckbox(i, 2, "Bot", playerManager.GetActivePlayer(i)->Bot);

            if (MenuButton(i, 1, "Drop Out")) {
                playerManager.SaveSpecificPlayer(i);
                playerManager.RemoveActivePlayer(i);
                OvershellState[i] = OS_ATTRACT;
                CanMouseClick = true;
                continue;
            }
            if (MenuButton(i, 0, "Cancel")) {
                playerManager.SaveSpecificPlayer(i);
                OvershellState[i] = OS_ATTRACT;
                CanMouseClick = true;
            }
            break;
        }
        case OS_ATTRACT: {
            if (playerManager.ActivePlayers[i] != -1) {
                // player active
                DrawBeacon(
                    i,
                    OvershellLeftLoc,
                    InnerBottom,
                    HalfWidth * 2,
                    GetScreenHeight(),
                    false
                );
                Color headerUsernameColor = playerManager.GetActivePlayer(i)->Bot ? SKYBLUE : WHITE;
                if (DrawOvershellRectangleHeader(
                        OvershellLeftLoc,
                        OvershellTopLoc,
                        unit.winpct(0.2f),
                        unit.winpct(0.05f),
                        playerManager.GetActivePlayer(i)->Name,
                        playerManager.GetActivePlayer(i)->AccentColor,
                        headerUsernameColor
                    )) {
                    OvershellState[i] = OS_OPTIONS;
                    CanMouseClick = false;
                    continue;
                    // playerManager.RemoveActivePlayer(i);
                    gameMenu.shouldBreak = true;
                };
            } else { // no active players
                // if its the first slot, keyboard can join ALWAYS
                if (IsGamepadAvailable(i) || i == 0) {
                    if (DrawOvershellRectangleHeader(
                            OvershellLeftLoc,
                            OvershellTopLoc + unit.winpct(0.01f),
                            unit.winpct(0.2f),
                            unit.winpct(0.04f),
                            "JOIN",
                            LIGHTGRAY,
                            RAYWHITE
                        )) {
                        CanMouseClick = false;
                        OvershellState[i] = OS_PLAYER_SELECTION;
                        gameMenu.shouldBreak = true;
                        continue;
                    };
                } else {
                    if (DrawOvershellRectangleHeader(
                            OvershellLeftLoc,
                            OvershellTopLoc + unit.winpct(0.01f),
                            unit.winpct(0.2f),
                            unit.winpct(0.04f),
                            "CONNECT CONTROLLER",
                            { 0 },
                            LIGHTGRAY
                        )) {
                        CanMouseClick = false;
                        OvershellState[i] = OS_PLAYER_SELECTION;
                        gameMenu.shouldBreak = true;
                        continue;
                    };
                    ;
                }
            }

            break;
        }
        case OS_INSTRUMENT_SELECTIONS: {
            int ButtonHeight = unit.winpct(0.03f);
            Color headerUsernameColor = playerManager.GetActivePlayer(i)->Bot ? SKYBLUE : WHITE;
            DrawOvershellRectangleHeader(
                OvershellLeftLoc,
                OvershellTopLoc - (ButtonHeight * 3),
                unit.winpct(0.2f),
                unit.winpct(0.05f),
                playerManager.GetActivePlayer(i)->Name,
                playerManager.GetActivePlayer(i)->AccentColor,
                headerUsernameColor
            );

            playerManager.GetActivePlayer(i)->ClassicMode = OvershellCheckbox(i, 1, "Classic", playerManager.GetActivePlayer(i)->ClassicMode);

            playerManager.GetActivePlayer(i)->ProDrums = OvershellCheckbox(i, 2, "Pro Drums", playerManager.GetActivePlayer(i)->ProDrums);


            if (MenuButton(i, 0, "Back")) {
                OvershellState[i] = OS_OPTIONS;
                continue;
            }
            break;
        }
        }
    }
};

*/
