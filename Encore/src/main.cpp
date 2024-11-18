#define JSON_DIAGNOSTICS 1
#include "menus/GameplayMenu.h"
#include "song/songlist.h"
#include "users/playerManager.h"
#include "menus/menu.h"
#include "menus/OvershellMenu.h"
#include "menus/sndTestMenu.h"
#include "menus/cacheLoadingScreen.h"
#include "menus/resultsMenu.h"
#include "util/enclog.h"
#include "gameplay/enctime.h"

#include "song/song.h"

#define RAYGUI_IMPLEMENTATION

/* not needed for debug purposes
#if defined(WIN32) && defined(NDEBUG)
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif
*/
#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#endif

#include <filesystem>
#include <iostream>
#include <vector>
#include <thread>
#include <condition_variable>
#include <thread>
#include <atomic>

#include "raylib.h"
#include "raygui.h"
#include "raymath.h"
#include "GLFW/glfw3.h"

#include "arguments.h"
#include "assets.h"
#include "song/audio.h"
#include "gameplay/gameplayRenderer.h"
#include "keybinds.h"
#include "old/lerp.h"
#include "menus/gameMenu.h"
#include "menus/overshellRenderer.h"

#include "menus/settingsOptionRenderer.h"

#include "menus/uiUnits.h"

#include "settings-old.h"
#include "timingvalues.h"
#include "gameplay/InputHandler.h"
#include "inih/INIReader.h"
#include "menus/ChartLoadingMenu.h"
#include "menus/ReadyUpMenu.h"
#include "menus/SettingsMenu.h"
#include "menus/SongSelectMenu.h"

#include "menus/styles.h"
#include <menus/MenuManager.h>

MenuManager TheMenuManager;
gameplayRenderer TheGameRenderer;
SongList TheSongList;
PlayerManager ThePlayerManager;
SettingsOld &settingsMain = SettingsOld::getInstance();
AudioManager &audioManager = AudioManager::getInstance();

// OvershellRenderer overshellRenderer;
InputHandler inputHandler;

Assets &assets = Assets::getInstance();

vector<std::string> ArgumentList::arguments;

#ifndef GIT_COMMIT_HASH
#define GIT_COMMIT_HASH
#endif

#ifndef ENCORE_VERSION
#define ENCORE_VERSION
#endif

// calibration
bool isCalibrating = false;
double calibrationStartTime = 0.0;
double lastClickTime = 0.0;
std::vector<double> tapTimes;
const int clickInterval = 1;

bool showInputFeedback = false;
double inputFeedbackStartTime = 0.0;
const double inputFeedbackDuration = 0.6;
float inputFeedbackAlpha = 1.0f;
// end calibration

std::string encoreVersion = ENCORE_VERSION;
std::string commitHash = GIT_COMMIT_HASH;

bool Menu::onNewMenu = false;

/*
std::string scoreCommaFormatter(int value) {
    std::stringstream ss;
    ss.imbue(std::locale(std::cout.getloc(), new Separators<char>()));
    ss << std::fixed << value;
    return ss.str();
}
*/

// what to check when a key changes states (what was the change? was it pressed? or
// released? what time? what window? were any modifiers pressed?)
static void keyCallback(GLFWwindow *wind, int key, int scancode, int action, int mods) {
    Player *player = ThePlayerManager.GetActivePlayer(0);
    PlayerGameplayStats *stats = player->stats;
    if (!TheGameRenderer.streamsLoaded) {
        return;
    }
    if (action < 2) {
        // if the key action is NOT repeat (release is 0, press is 1)
        int lane = -2;
        if (key == settingsMain.keybindPause && action == GLFW_PRESS) {
            stats->Paused = !stats->Paused;
            if (stats->Paused && !ThePlayerManager.BandStats.Multiplayer) {
                audioManager.pauseStreams();
                TheSongTime.Pause();
            } else if (!ThePlayerManager.BandStats.Multiplayer) {
                audioManager.unpauseStreams();
                TheSongTime.Resume();
                for (int i = 0; i < (player->Difficulty == 3 ? 5 : 4); i++) {
                    inputHandler.handleInputs(player, i, -1);
                }
            } // && !player->Bot
        } else if ((key == settingsMain.keybindOverdrive
                    || key == settingsMain.keybindOverdriveAlt)) {
            inputHandler.handleInputs(player, -1, action);
        } else if (!player->Bot) {
            if (player->Instrument != PLASTIC_DRUMS) {
                if (player->Difficulty == 3 || player->ClassicMode) {
                    for (int i = 0; i < 5; i++) {
                        if (key == settingsMain.keybinds5K[i]
                            && !stats->HeldFretsAlt[i]) {
                            if (action == GLFW_PRESS) {
                                stats->HeldFrets[i] = true;
                            } else if (action == GLFW_RELEASE) {
                                stats->HeldFrets[i] = false;
                                stats->OverhitFrets[i] = false;
                            }
                            lane = i;
                        } else if (key == settingsMain.keybinds5KAlt[i] && !stats->HeldFrets[i]) {
                            if (action == GLFW_PRESS) {
                                stats->HeldFretsAlt[i] = true;
                            } else if (action == GLFW_RELEASE) {
                                stats->HeldFretsAlt[i] = false;
                                stats->OverhitFrets[i] = false;
                            }
                            lane = i;
                        }
                    }
                } else {
                    for (int i = 0; i < 4; i++) {
                        if (key == settingsMain.keybinds4K[i]
                            && !stats->HeldFretsAlt[i]) {
                            if (action == GLFW_PRESS) {
                                stats->HeldFrets[i] = true;
                            } else if (action == GLFW_RELEASE) {
                                stats->HeldFrets[i] = false;
                                stats->OverhitFrets[i] = false;
                            }
                            lane = i;
                        } else if (key == settingsMain.keybinds4KAlt[i] && !stats->HeldFrets[i]) {
                            if (action == GLFW_PRESS) {
                                stats->HeldFretsAlt[i] = true;
                            } else if (action == GLFW_RELEASE) {
                                stats->HeldFretsAlt[i] = false;
                                stats->OverhitFrets[i] = false;
                            }
                            lane = i;
                        }
                    }
                }
                if (player->ClassicMode) {
                    if (key == settingsMain.keybindStrumUp) {
                        if (action == GLFW_PRESS) {
                            lane = 8008135;
                            stats->UpStrum = true;
                        } else if (action == GLFW_RELEASE) {
                            stats->UpStrum = false;
                            stats->Overstrum = false;
                        }
                    }
                    if (key == settingsMain.keybindStrumDown) {
                        if (action == GLFW_PRESS) {
                            lane = 8008135;
                            stats->DownStrum = true;
                        } else if (action == GLFW_RELEASE) {
                            stats->DownStrum = false;
                            stats->Overstrum = false;
                        }
                    }
                }

                if (lane != -1 && lane != -2) {
                    inputHandler.handleInputs(player, lane, action);
                }
            }
        }
    }
}

static void gamepadStateCallback(int joypadID, GLFWgamepadstate state) {
    Encore::EncoreLog(LOG_DEBUG, TextFormat("Attempted input on joystick %01i", joypadID));
    Player *player;
    if (ThePlayerManager.IsGamepadActive(joypadID))
        player = ThePlayerManager.GetPlayerGamepad(joypadID);
    else
        return;

    if (!IsGamepadAvailable(player->joypadID))
        return;
    PlayerGameplayStats *stats = player->stats;
    if (!TheGameRenderer.streamsLoaded) {
        return;
    }

    double eventTime = TheSongTime.GetSongTime();
    if (settingsMain.controllerPause >= 0) {
        if (state.buttons[settingsMain.controllerPause]
            != stats->buttonValues[settingsMain.controllerPause]) {
            stats->buttonValues[settingsMain.controllerPause] =
                state.buttons[settingsMain.controllerPause];
            if (state.buttons[settingsMain.controllerPause] == 1) {
                stats->Paused = !stats->Paused;
                if (stats->Paused && !ThePlayerManager.BandStats.Multiplayer) {
                    audioManager.pauseStreams();
                    TheSongTime.Pause();
                } else if (!ThePlayerManager.BandStats.Multiplayer) {
                    audioManager.unpauseStreams();
                    TheSongTime.Resume();
                    for (int i = 0; i < (player->Difficulty == 3 ? 5 : 4); i++) {
                        inputHandler.handleInputs(player, i, -1);
                    }
                } // && !player->Bot
            }
        }
    } else if (!player->Bot) {
        if (state.axes[-(settingsMain.controllerPause + 1)]
            != stats->axesValues[-(settingsMain.controllerPause + 1)]) {
            stats->axesValues[-(settingsMain.controllerPause + 1)] =
                state.axes[-(settingsMain.controllerPause + 1)];
            if (state.axes[-(settingsMain.controllerPause + 1)]
                == 1.0f * (float)settingsMain.controllerPauseAxisDirection) {
            }
        }
    } //  && !player->Bot
    if (settingsMain.controllerOverdrive >= 0) {
        if (state.buttons[settingsMain.controllerOverdrive]
            != stats->buttonValues[settingsMain.controllerOverdrive]) {
            stats->buttonValues[settingsMain.controllerOverdrive] =
                state.buttons[settingsMain.controllerOverdrive];
            inputHandler.handleInputs(
                player, -1, state.buttons[settingsMain.controllerOverdrive]
            );
        } // // if (!player->Bot)
    } else {
        if (state.axes[-(settingsMain.controllerOverdrive + 1)]
            != stats->axesValues[-(settingsMain.controllerOverdrive + 1)]) {
            stats->axesValues[-(settingsMain.controllerOverdrive + 1)] =
                state.axes[-(settingsMain.controllerOverdrive + 1)];
            if (state.axes[-(settingsMain.controllerOverdrive + 1)]
                == 1.0f * (float)settingsMain.controllerOverdriveAxisDirection) {
                inputHandler.handleInputs(player, -1, GLFW_PRESS);
            } else {
                inputHandler.handleInputs(player, -1, GLFW_RELEASE);
            }
        }
    }
    if ((player->Difficulty == 3 || player->ClassicMode) && !player->Bot) {
        int lane = -2;
        int action = -2;
        for (int i = 0; i < 5; i++) {
            if (settingsMain.controller5K[i] >= 0) {
                if (state.buttons[settingsMain.controller5K[i]]
                    != stats->buttonValues[settingsMain.controller5K[i]]) {
                    if (state.buttons[settingsMain.controller5K[i]] == 1
                        && !stats->HeldFrets[i])
                        stats->HeldFrets[i] = true;
                    else if (stats->HeldFrets[i]) {
                        stats->HeldFrets[i] = false;
                        stats->OverhitFrets[i] = false;
                    }
                    inputHandler.handleInputs(
                        player, i, state.buttons[settingsMain.controller5K[i]]
                    );
                    stats->buttonValues[settingsMain.controller5K[i]] =
                        state.buttons[settingsMain.controller5K[i]];
                    lane = i;
                }
            } else {
                if (state.axes[-(settingsMain.controller5K[i] + 1)]
                    != stats->axesValues[-(settingsMain.controller5K[i] + 1)]) {
                    if (state.axes[-(settingsMain.controller5K[i] + 1)]
                            == 1.0f * (float)settingsMain.controller5KAxisDirection[i]
                        && !stats->HeldFrets[i]) {
                        stats->HeldFrets[i] = true;
                        inputHandler.handleInputs(player, i, GLFW_PRESS);
                    } else if (stats->HeldFrets[i]) {
                        stats->HeldFrets[i] = false;
                        stats->OverhitFrets[i] = false;
                        inputHandler.handleInputs(player, i, GLFW_RELEASE);
                    }
                    stats->axesValues[-(settingsMain.controller5K[i] + 1)] =
                        state.axes[-(settingsMain.controller5K[i] + 1)];
                    lane = i;
                }
            }
        }

        if (state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP] == GLFW_PRESS
            && player->ClassicMode && !stats->UpStrum) {
            stats->UpStrum = true;
            stats->Overstrum = false;
            inputHandler.handleInputs(player, 8008135, GLFW_PRESS);
        } else if (state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP] == GLFW_RELEASE
                   && player->ClassicMode
                   && stats->UpStrum) {
            stats->UpStrum = false;
            inputHandler.handleInputs(player, 8008135, GLFW_RELEASE);
        }
        if (state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN] == GLFW_PRESS
            && player->ClassicMode && !stats->DownStrum) {
            stats->DownStrum = true;
            stats->Overstrum = false;
            inputHandler.handleInputs(player, 8008135, GLFW_PRESS);
        } else if (state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN] == GLFW_RELEASE
                   && player->ClassicMode
                   && stats->DownStrum) {
            stats->DownStrum = false;
            inputHandler.handleInputs(player, 8008135, GLFW_RELEASE);
        }
    } else if (!player->Bot) {
        for (int i = 0; i < 4; i++) {
            if (settingsMain.controller4K[i] >= 0) {
                if (state.buttons[settingsMain.controller4K[i]]
                    != stats->buttonValues[settingsMain.controller4K[i]]) {
                    if (state.buttons[settingsMain.controller4K[i]] == 1)
                        stats->HeldFrets[i] = true;
                    else {
                        stats->HeldFrets[i] = false;
                        stats->OverhitFrets[i] = false;
                    }
                    inputHandler.handleInputs(
                        player, i, state.buttons[settingsMain.controller4K[i]]
                    );
                    stats->buttonValues[settingsMain.controller4K[i]] =
                        state.buttons[settingsMain.controller4K[i]];
                }
            } else {
                if (state.axes[-(settingsMain.controller4K[i] + 1)]
                    != stats->axesValues[-(settingsMain.controller4K[i] + 1)]) {
                    if (state.axes[-(settingsMain.controller4K[i] + 1)]
                        == 1.0f * (float)settingsMain.controller4KAxisDirection[i]) {
                        stats->HeldFrets[i] = true;
                        inputHandler.handleInputs(player, i, GLFW_PRESS);
                    } else {
                        stats->HeldFrets[i] = false;
                        stats->OverhitFrets[i] = false;
                        inputHandler.handleInputs(player, i, GLFW_RELEASE);
                    }
                    stats->axesValues[-(settingsMain.controller4K[i] + 1)] =
                        state.axes[-(settingsMain.controller4K[i] + 1)];
                }
            }
        }
    }
}
/*
static void gamepadStateCallbackSetControls(int jid, GLFWgamepadstate state) {
    for (int i = 0; i < 6; i++) {
        axesValues2[i] = state.axes[i];
    }
    if (changingKey || changingOverdrive || changingPause) {
        for (int i = 0; i < 15; i++) {
            if (state.buttons[i] == 1) {
                if (buttonValues[i] == 0) {
                    controllerID = jid;
                    pressedGamepadInput = i;
                    return;
                } else {
                    buttonValues[i] = state.buttons[i];
                }
            }
        }
        for (int i = 0; i < 6; i++) {
            if (state.axes[i] == 1.0f || (i <= 3 && state.axes[i] == -1.0f)) {
                axesValues[i] = 0.0f;
                if (state.axes[i] == 1.0f) axisDirection = 1;
                else axisDirection = -1;
                controllerID = jid;
                pressedGamepadInput = -(1 + i);
                return;
            } else {
                axesValues[i] = 0.0f;
            }
        }
    } else {
        for (int i = 0; i < 15; i++) {
            buttonValues[i] = state.buttons[i];
        }
        for (int i = 0; i < 6; i++) {
            axesValues[i] = state.axes[i];
        }
        pressedGamepadInput = -999;
    }
}
*/
int minWidth = 640;
int minHeight = 480;


bool firstInit = true;
int loadedAssets;
bool albumArtLoaded = false;

Menu *ActiveMenu = nullptr;


bool doRenderThingToLowerHighway = false;

int CurSongInt = 0;

int main(int argc, char *argv[]) {
    SetTraceLogCallback(Encore::EncoreLog);
    Units u = Units::getInstance();
    commitHash.erase(7);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    SetWindowState(FLAG_MSAA_4X_HINT);
    bool windowToggle = true;
    ArgumentList::InitArguments(argc, argv);

    std::string FPSCapStringVal = ArgumentList::GetArgValue("fpscap");
    std::string vSyncOn = ArgumentList::GetArgValue("vsync");
    int targetFPSArg = -1;
    int vsyncArg = 1;

    if (!FPSCapStringVal.empty()) {
        targetFPSArg = strtol(FPSCapStringVal.c_str(), NULL, 10);
        if (targetFPSArg > 0)
            Encore::EncoreLog(
                LOG_INFO, TextFormat("Argument overridden target FPS: %d", targetFPSArg)
            );
        else
            Encore::EncoreLog(LOG_INFO, TextFormat("Unlocked framerate."));
    }

    if (!vSyncOn.empty()) {
        vsyncArg = strtol(vSyncOn.c_str(), NULL, 10);
        Encore::EncoreLog(
            LOG_INFO, TextFormat("Vertical sync argument toggled: %d", vsyncArg)
        );
    }
    if (vsyncArg == 1) {
        SetConfigFlags(FLAG_VSYNC_HINT);
    }

    std::filesystem::path executablePath(GetApplicationDirectory());

    std::filesystem::path directory = executablePath.parent_path();

#ifdef __APPLE__
    CFBundleRef bundle = CFBundleGetMainBundle();
    if (bundle != NULL) {
        // get the Resources directory for our binary for the Assets handling
        CFURLRef resourceURL = CFBundleCopyResourcesDirectoryURL(bundle);
        if (resourceURL != NULL) {
            char resourcePath[PATH_MAX];
            if (CFURLGetFileSystemRepresentation(
                    resourceURL, true, (UInt8 *)resourcePath, PATH_MAX
                ))
                assets.setDirectory(resourcePath);
            CFRelease(resourceURL);
        }
        // do the next step manually (settings/config handling)
        // "directory" is our executable directory here, hop up to the external dir
        if (directory.filename().compare("MacOS") == 0)
            directory = directory.parent_path().parent_path().parent_path(); // hops
        // "MacOS",
        // "Contents",
        // "Encore.app"
        // into
        // containing
        // folder

        CFRelease(bundle);
    }
#endif
    settingsMain.setDirectory(directory);
    ThePlayerManager.SetPlayerListSaveFileLocation(directory / "players.json");
    if (std::filesystem::exists(directory / "keybinds.json")) {
        settingsMain.migrateSettings(
            directory / "keybinds.json", directory / "settings.json"
        );
    }
    settingsMain.loadSettings(directory / "settings.json");
    ThePlayerManager.LoadPlayerList();

    bool removeFPSLimit = 0;
    int targetFPS =
        targetFPSArg == -1 ? GetMonitorRefreshRate(GetCurrentMonitor()) : targetFPSArg;
    removeFPSLimit = targetFPSArg == 0;
    int menuFPS = GetMonitorRefreshRate(GetCurrentMonitor()) / 2;

    // https://www.raylib.com/examples/core/loader.html?name=core_custom_frame_control

    double previousTime = GetTime();
    double currentTime = 0.0;
    double updateDrawTime = 0.0;
    double waitTime = 0.0;
    float deltaTime = 0.0;

    if (!settingsMain.fullscreen) {
        InitWindow(
            GetMonitorWidth(GetCurrentMonitor()) * 0.75f,
            GetMonitorHeight(GetCurrentMonitor()) * 0.75f,
            "Encore"
        );
        SET_WINDOW_WINDOWED();
    } else {
        InitWindow(
            GetMonitorWidth(GetCurrentMonitor()),
            GetMonitorHeight(GetCurrentMonitor()),
            "Encore"
        );
        SET_WINDOW_FULLSCREEN_BORDERLESS();
    }

    Encore::EncoreLog(LOG_INFO, TextFormat("Target FPS: %d", targetFPS));

    audioManager.Init();
    SetExitKey(0);
    audioManager.loadSample("Assets/combobreak.mp3", "miss");

    ChangeDirectory(GetApplicationDirectory());

    GLFWkeyfun origKeyCallback = glfwSetKeyCallback(glfwGetCurrentContext(), keyCallback);
    GLFWgamepadstatefun origGamepadCallback =
        glfwSetGamepadStateCallback(gamepadStateCallback);
    glfwSetKeyCallback(glfwGetCurrentContext(), origKeyCallback);
    glfwSetGamepadStateCallback(origGamepadCallback);

    SETDEFAULTSTYLE();

    SetRandomSeed(std::chrono::system_clock::now().time_since_epoch().count());
    assets.FirstAssets();
    SetWindowIcon(assets.icon);
    GuiSetFont(assets.rubik);
    assets.LoadAssets();
    TheMenuManager.currentScreen = CACHE_LOADING_SCREEN;
    Menu::onNewMenu = true;
    TheSongTime.SetOffset(settingsMain.avOffsetMS / 1000.0);

    audioManager.loadSample("Assets/highway/clap.mp3", "clap");
    while (!WindowShouldClose()) {
        u.calcUnits();
        double curTime = GetTime();
        float bgTime = curTime / 5.0f;
        if (IsKeyPressed(KEY_F11)
            || (IsKeyPressed(KEY_LEFT_ALT) && IsKeyPressed(KEY_ENTER))) {
            settingsMain.fullscreen = !settingsMain.fullscreen;
            if (!settingsMain.fullscreen) {
                SET_WINDOW_WINDOWED();
            } else {
                SET_WINDOW_FULLSCREEN_BORDERLESS();
            }
        }
        if (GetScreenWidth() < minWidth) {
            if (GetScreenHeight() < minHeight)
                SetWindowSize(minWidth, minHeight);
            else
                SetWindowSize(minWidth, GetScreenHeight());
        }
        if (GetScreenHeight() < minHeight) {
            if (GetScreenWidth() < minWidth)
                SetWindowSize(minWidth, minHeight);
            else
                SetWindowSize(GetScreenWidth(), minHeight);
        }

        BeginDrawing();
        ClearBackground(DARKGRAY);
        SetShaderValue(assets.bgShader, assets.bgTimeLoc, &bgTime, SHADER_UNIFORM_FLOAT);

        if (TheMenuManager.onNewMenu) {
            TheMenuManager.onNewMenu = false;
            delete ActiveMenu;
            ActiveMenu = NULL;
            // this is for dropping out
            glfwSetKeyCallback(glfwGetCurrentContext(), origKeyCallback);
            glfwSetGamepadStateCallback(origGamepadCallback);
            switch (TheMenuManager.currentScreen) { // NOTE: when adding a new Menu
                                                    // derivative, you
                // must put its enum value in Screens, and its
                // assignment in this switch/case. You will also
                // add its case to the `ActiveMenu->Draw();`
                // cases.
            case MAIN_MENU: {
                ActiveMenu = new MainMenu;
                ActiveMenu->Load();
                break;
            }
            case SETTINGS: {
                ActiveMenu = new SettingsMenu;
                ActiveMenu->Load();
                break;
            }
            case RESULTS: {
                ActiveMenu = new resultsMenu;
                ActiveMenu->Load();
                break;
            }
            case SONG_SELECT: {
                ActiveMenu = new SongSelectMenu;
                ActiveMenu->Load();
                break;
            }
            case READY_UP: {
                ActiveMenu = new ReadyUpMenu;
                ActiveMenu->Load();
                break;
            }
            case SOUND_TEST: {
                ActiveMenu = new SoundTestMenu;
                ActiveMenu->Load();
                break;
            }
            case CACHE_LOADING_SCREEN: {
                ActiveMenu = new cacheLoadingScreen;
                ActiveMenu->Load();
                break;
            }
            case CHART_LOADING_SCREEN: {
                ActiveMenu = new ChartLoadingMenu;
                ActiveMenu->Load();
                break;
            }
            case GAMEPLAY: {
                glfwSetKeyCallback(glfwGetCurrentContext(), keyCallback);
                glfwSetGamepadStateCallback(gamepadStateCallback);
                ActiveMenu = new GameplayMenu;
                ActiveMenu->Load();
                break;
            }
            default:;
            }
        }

        switch (TheMenuManager.currentScreen) {
        case CALIBRATION: {
            static bool sampleLoaded = false;
            if (!sampleLoaded) {
                audioManager.loadSample("Assets/kick.wav", "click");
                sampleLoaded = true;
            }

            if (GuiButton(
                    { (float)GetScreenWidth() / 2 - 250,
                      (float)GetScreenHeight() - 120,
                      200,
                      60 },
                    "Start Calibration"
                )) {
                isCalibrating = true;
                calibrationStartTime = GetTime();
                lastClickTime = calibrationStartTime;
                tapTimes.clear();
            }
            if (GuiButton(
                    { (float)GetScreenWidth() / 2 + 50,
                      (float)GetScreenHeight() - 120,
                      200,
                      60 },
                    "Stop Calibration"
                )) {
                isCalibrating = false;

                if (tapTimes.size() > 1) {
                    double totalDifference = 0.0;
                    for (double tapTime : tapTimes) {
                        double expectedClickTime =
                            round((tapTime - calibrationStartTime) / clickInterval)
                                * clickInterval
                            + calibrationStartTime;
                        totalDifference += (tapTime - expectedClickTime);
                    }
                    settingsMain.avOffsetMS =
                        static_cast<int>((totalDifference / tapTimes.size()) * 1000);
                    // Convert to milliseconds
                    settingsMain.inputOffsetMS = settingsMain.avOffsetMS;
                    std::cout
                        << static_cast<int>((totalDifference / tapTimes.size()) * 1000)
                        << "ms of latency detected" << std::endl;
                }
                std::cout << "Stopped Calibration" << std::endl;
                tapTimes.clear();
            }

            if (isCalibrating) {
                double currentTime = GetTime();
                double elapsedTime = currentTime - lastClickTime;

                if (elapsedTime >= clickInterval) {
                    audioManager.playSample("click", 1);
                    lastClickTime += clickInterval;
                    // Increment by the interval to avoid missing clicks
                    std::cout << "Click" << std::endl;
                }

                if (IsKeyPressed(settingsMain.keybindOverdrive)) {
                    tapTimes.push_back(currentTime);
                    std::cout << "Input Registered" << std::endl;

                    showInputFeedback = true;
                    inputFeedbackStartTime = currentTime;
                    inputFeedbackAlpha = 1.0f;
                }
            }

            if (showInputFeedback) {
                double currentTime = GetTime();
                double timeSinceInput = currentTime - inputFeedbackStartTime;
                if (timeSinceInput > inputFeedbackDuration) {
                    showInputFeedback = false;
                } else {
                    inputFeedbackAlpha = 1.0f - (timeSinceInput / inputFeedbackDuration);
                }
            }

            if (showInputFeedback) {
                Color feedbackColor = {
                    0, 255, 0, static_cast<unsigned char>(inputFeedbackAlpha * 255)
                };
                DrawTextEx(
                    assets.rubikBold,
                    "Input Registered",
                    { static_cast<float>((GetScreenWidth() - u.hinpct(0.35f)) / 2),
                      static_cast<float>(GetScreenHeight() / 2) },
                    u.hinpct(0.05f),
                    0,
                    feedbackColor
                );
            }

            if (GuiButton(
                    { ((float)GetScreenWidth() / 2) - 350,
                      ((float)GetScreenHeight() - 60),
                      100,
                      60 },
                    "Cancel"
                )) {
                isCalibrating = false;
                settingsMain.avOffsetMS = settingsMain.prevAvOffsetMS;
                settingsMain.inputOffsetMS = settingsMain.prevInputOffsetMS;
                tapTimes.clear();

                settingsMain.saveSettings(directory / "settings.json");
                TheMenuManager.SwitchScreen(SETTINGS);
            }

            if (GuiButton(
                    { ((float)GetScreenWidth() / 2) + 250,
                      ((float)GetScreenHeight() - 60),
                      100,
                      60 },
                    "Apply"
                )) {
                isCalibrating = false;
                settingsMain.prevAvOffsetMS = settingsMain.avOffsetMS;
                settingsMain.prevInputOffsetMS = settingsMain.inputOffsetMS;
                tapTimes.clear();

                settingsMain.saveSettings(directory / "settings.json");
                TheMenuManager.SwitchScreen(SETTINGS);
            }

            break;
        }
        case MAIN_MENU:
        case SETTINGS:
        case SONG_SELECT:
        case READY_UP:
        case GAMEPLAY:
        case RESULTS:
        case CHART_LOADING_SCREEN:
        case SOUND_TEST:
        case CACHE_LOADING_SCREEN: {
            ActiveMenu->Draw();
            break;
        }
        }
        EndDrawing();

        if (!removeFPSLimit || TheMenuManager.currentScreen != GAMEPLAY) {
            currentTime = GetTime();
            updateDrawTime = currentTime - previousTime;
            int Target = targetFPS;
            if (TheMenuManager.currentScreen != GAMEPLAY)
                Target = menuFPS;

            if (Target > 0) {
                waitTime = (1.0f / (float)Target) - updateDrawTime;
                if (waitTime > 0.0) {
                    WaitTime((float)waitTime);
                    currentTime = GetTime();
                    deltaTime = (float)(currentTime - previousTime);
                }
            } else
                deltaTime = (float)updateDrawTime;

            previousTime = currentTime;
        }
    }

    CloseWindow();
    return 0;
}
