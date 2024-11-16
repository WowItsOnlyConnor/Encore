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
#include "menus/ReadyUpMenu.h"
#include "menus/SongSelectMenu.h"

#include "menus/styles.h"
#include <menus/MenuManager.h>

MenuManager TheMenuManager;
SongTime &enctime = TheSongTime;
gameplayRenderer TheGameRenderer;
SongList TheSongList;
PlayerManager ThePlayerManager;
SettingsOld &settingsMain = SettingsOld::getInstance();
AudioManager &audioManager = AudioManager::getInstance();

// OvershellRenderer overshellRenderer;
InputHandler inputHandler;
Keybinds keybinds;
settingsOptionRenderer sor;

Assets &assets = Assets::getInstance();

vector<std::string> ArgumentList::arguments;

enum OptionsCategories {
    MAIN,
    HIGHWAY,
    VOLUME,
    KEYBOARD,
    GAMEPAD
};

enum KeybindCategories {
    kbPAD,
    kbCLASSIC,
    kbMISC,
    kbMENUS
};

enum JoybindCategories {
    gpPAD,
    gpCLASSIC,
    gpMISC,
    gpMENUS
};

#ifndef GIT_COMMIT_HASH
#define GIT_COMMIT_HASH
#endif

#ifndef ENCORE_VERSION
#define ENCORE_VERSION
#endif

bool ShowHighwaySettings = true;
bool ShowCalibrationSettings = true;
bool ShowGeneralSettings = true;

bool selSong = false;
bool changingKey = false;
bool changing4k = false;
bool changingOverdrive = false;
bool changingAlt = false;
bool changingPause = false;

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

void Windowed() {
    ClearWindowState(FLAG_WINDOW_UNDECORATED);
    SetWindowSize(
        GetMonitorWidth(GetCurrentMonitor()) * 0.75f,
        GetMonitorHeight(GetCurrentMonitor()) * 0.75f
    );
    SetWindowPosition(
        (GetMonitorWidth(GetCurrentMonitor()) * 0.5f)
            - (GetMonitorWidth(GetCurrentMonitor()) * 0.375f),
        (0.5f * GetMonitorHeight(GetCurrentMonitor()))
            - (GetMonitorHeight(GetCurrentMonitor()) * 0.375f)
    );
}

void FullscreenBorderless() {
    SetWindowSize(
        GetMonitorWidth(GetCurrentMonitor()), GetMonitorHeight(GetCurrentMonitor())
    );
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    SetWindowPosition(0, 0);
}

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
                enctime.Pause();
            } else if (!ThePlayerManager.BandStats.Multiplayer) {
                audioManager.unpauseStreams();
                enctime.Resume();
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

    double eventTime = enctime.GetSongTime();
    if (settingsMain.controllerPause >= 0) {
        if (state.buttons[settingsMain.controllerPause]
            != stats->buttonValues[settingsMain.controllerPause]) {
            stats->buttonValues[settingsMain.controllerPause] =
                state.buttons[settingsMain.controllerPause];
            if (state.buttons[settingsMain.controllerPause] == 1) {
                stats->Paused = !stats->Paused;
                if (stats->Paused && !ThePlayerManager.BandStats.Multiplayer) {
                    audioManager.pauseStreams();
                    enctime.Pause();
                } else if (!ThePlayerManager.BandStats.Multiplayer) {
                    audioManager.unpauseStreams();
                    enctime.Resume();
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

bool FinishedLoading = false;
bool firstInit = true;
int loadedAssets;
bool albumArtLoaded = false;

Menu *ActiveMenu = nullptr;

void LoadCharts() {
    smf::MidiFile midiFile;
    midiFile.read(TheSongList.curSong->midiPath.string());
    TheSongList.curSong->getTiming(midiFile, 0, midiFile[0]);
    for (int playerNum = 0; playerNum < ThePlayerManager.PlayersActive; playerNum++) {
        Player *player = ThePlayerManager.GetActivePlayer(playerNum);
        int diff = player->Difficulty;
        int inst = player->Instrument;
        int track = TheSongList.curSong->parts[inst]->charts[diff].track;
        std::string trackName;

        Chart &chart = TheSongList.curSong->parts[inst]->charts[diff];
        if (chart.valid) {
            Encore::EncoreLog(
                LOG_DEBUG,
                TextFormat("Loading part %s, diff %01i", trackName.c_str(), diff)
            );
            LoadingState = NOTE_PARSING;
            if (inst < PitchedVocals && inst != PlasticDrums && inst > PartVocals) {
                chart.plastic = true;
                chart.parsePlasticNotes(
                    midiFile, track, diff, inst, TheSongList.curSong->hopoThreshold
                );
            } else if (inst == PlasticDrums) {
                chart.plastic = true;
                chart.parsePlasticDrums(
                    midiFile, track, midiFile[track], diff, inst, player->ProDrums, true
                );
            } else {
                chart.plastic = false;
                chart.parseNotes(midiFile, track, midiFile[track], diff, inst);
            }

            if (!chart.plastic) {
                LoadingState = EXTRA_PROCESSING;
                int noteIdx = 0;
                for (Note &note : chart.notes) {
                    chart.notes_perlane[note.lane].push_back(noteIdx);
                    noteIdx++;
                }
            }
        }

        //}
        //}
        //}
    }

    TheSongList.curSong->getCodas(midiFile);
    LoadingState = READY;
    this_thread::sleep_for(chrono::seconds(1));
    FinishedLoading = true;
}

bool StartLoading = true;

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
        Windowed();
    } else {
        InitWindow(
            GetMonitorWidth(GetCurrentMonitor()),
            GetMonitorHeight(GetCurrentMonitor()),
            "Encore"
        );
        FullscreenBorderless();
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
    enctime.SetOffset(settingsMain.avOffsetMS / 1000.0);

    audioManager.loadSample("Assets/highway/clap.mp3", "clap");
    while (!WindowShouldClose()) {
        u.calcUnits();
        double curTime = GetTime();
        float bgTime = curTime / 5.0f;
        if (IsKeyPressed(KEY_F11)
            || (IsKeyPressed(KEY_LEFT_ALT) && IsKeyPressed(KEY_ENTER))) {
            settingsMain.fullscreen = !settingsMain.fullscreen;
            if (!settingsMain.fullscreen) {
                Windowed();
            } else {
                FullscreenBorderless();
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
        case MAIN_MENU: {
            ActiveMenu->Draw();
            break;
        }
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
        case SETTINGS: {
            GameMenu::DrawAlbumArtBackground(TheSongList.curSong->albumArtBlur);
            // if (settingsMain.controllerType == -1 && controllerID != -1) {
            //	std::string gamepadName = std::string(glfwGetGamepadName(controllerID));
            //	settingsMain.controllerType = keybinds.getControllerType(gamepadName);
            //}
            float TextPlacementTB = u.hpct(0.15f) - u.hinpct(0.11f);
            float TextPlacementLR = u.wpct(0.01f);
            DrawRectangle(
                u.LeftSide, 0, u.winpct(1.0f), GetScreenHeight(), Color { 0, 0, 0, 128 }
            );
            DrawLineEx(
                { u.LeftSide + u.winpct(0.0025f), 0 },
                { u.LeftSide + u.winpct(0.0025f), (float)GetScreenHeight() },
                u.winpct(0.005f),
                WHITE
            );
            DrawLineEx(
                { u.RightSide - u.winpct(0.0025f), 0 },
                { u.RightSide - u.winpct(0.0025f), (float)GetScreenHeight() },
                u.winpct(0.005f),
                WHITE
            );

            encOS::DrawTopOvershell(0.15f);
            GameMenu::DrawVersion();
            GameMenu::DrawBottomOvershell();
            DrawTextEx(
                assets.redHatDisplayBlack,
                "Options",
                { TextPlacementLR, TextPlacementTB },
                u.hinpct(0.10f),
                0,
                WHITE
            );

            float OvershellBottom = u.hpct(0.15f);
            if (GuiButton(
                    { ((float)GetScreenWidth() / 2) - 350,
                      ((float)GetScreenHeight() - 60),
                      100,
                      60 },
                    "Cancel"
                )
                && !(changingKey || changingOverdrive || changingPause)) {
                glfwSetGamepadStateCallback(origGamepadCallback);
                settingsMain.keybinds4K = settingsMain.prev4K;
                settingsMain.keybinds5K = settingsMain.prev5K;
                settingsMain.keybinds4KAlt = settingsMain.prev4KAlt;
                settingsMain.keybinds5KAlt = settingsMain.prev5KAlt;
                settingsMain.keybindOverdrive = settingsMain.prevOverdrive;
                settingsMain.keybindOverdriveAlt = settingsMain.prevOverdriveAlt;
                settingsMain.keybindPause = settingsMain.prevKeybindPause;

                settingsMain.controller4K = settingsMain.prevController4K;
                settingsMain.controller4KAxisDirection =
                    settingsMain.prevController4KAxisDirection;
                settingsMain.controller5K = settingsMain.prevController5K;
                settingsMain.controller5KAxisDirection =
                    settingsMain.prevController5KAxisDirection;
                settingsMain.controllerOverdrive = settingsMain.prevControllerOverdrive;
                settingsMain.controllerOverdriveAxisDirection =
                    settingsMain.prevControllerOverdriveAxisDirection;
                settingsMain.controllerType = settingsMain.prevControllerType;
                settingsMain.controllerPause = settingsMain.prevControllerPause;

                settingsMain.highwayLengthMult = settingsMain.prevHighwayLengthMult;
                settingsMain.trackSpeed = settingsMain.prevTrackSpeed;
                settingsMain.inputOffsetMS = settingsMain.prevInputOffsetMS;
                settingsMain.avOffsetMS = settingsMain.prevAvOffsetMS;
                settingsMain.missHighwayColor = settingsMain.prevMissHighwayColor;
                settingsMain.mirrorMode = settingsMain.prevMirrorMode;
                settingsMain.fullscreen = settingsMain.fullscreenPrev;

                settingsMain.MainVolume = settingsMain.prevMainVolume;
                settingsMain.PlayerVolume = settingsMain.prevPlayerVolume;
                settingsMain.BandVolume = settingsMain.prevBandVolume;
                settingsMain.SFXVolume = settingsMain.prevSFXVolume;
                settingsMain.MissVolume = settingsMain.prevMissVolume;
                settingsMain.MenuVolume = settingsMain.prevMenuVolume;
                enctime.SetOffset(settingsMain.avOffsetMS / 1000.0);
                TheMenuManager.SwitchScreen(MAIN_MENU);
            }
            if (GuiButton(
                    { ((float)GetScreenWidth() / 2) + 250,
                      ((float)GetScreenHeight() - 60),
                      100,
                      60 },
                    "Apply"
                )
                && !(changingKey || changingOverdrive || changingPause)) {
                glfwSetGamepadStateCallback(origGamepadCallback);
                if (settingsMain.fullscreen) {
                    FullscreenBorderless();
                    // SetWindowState(FLAG_WINDOW_UNDECORATED);
                    // SetWindowState(FLAG_MSAA_4X_HINT);
                    // int CurrentMonitor = GetCurrentMonitor();
                    // SetWindowPosition(0, 0);
                    // SetWindowSize(GetMonitorWidth(CurrentMonitor),
                    //			GetMonitorHeight(CurrentMonitor));
                } else {
                    Windowed();
                }
                settingsMain.prev4K = settingsMain.keybinds4K;
                settingsMain.prev5K = settingsMain.keybinds5K;
                settingsMain.prev4KAlt = settingsMain.keybinds4KAlt;
                settingsMain.prev5KAlt = settingsMain.keybinds5KAlt;
                settingsMain.prevOverdrive = settingsMain.keybindOverdrive;
                settingsMain.prevOverdriveAlt = settingsMain.keybindOverdriveAlt;
                settingsMain.prevKeybindPause = settingsMain.keybindPause;

                settingsMain.prevController4K = settingsMain.controller4K;
                settingsMain.prevController4KAxisDirection =
                    settingsMain.controller4KAxisDirection;
                settingsMain.prevController5K = settingsMain.controller5K;
                settingsMain.prevController5KAxisDirection =
                    settingsMain.controller5KAxisDirection;
                settingsMain.prevControllerOverdrive = settingsMain.controllerOverdrive;
                settingsMain.prevControllerPause = settingsMain.controllerPause;
                settingsMain.prevControllerOverdriveAxisDirection =
                    settingsMain.controllerOverdriveAxisDirection;
                settingsMain.prevControllerType = settingsMain.controllerType;

                settingsMain.prevHighwayLengthMult = settingsMain.highwayLengthMult;
                settingsMain.prevTrackSpeed = settingsMain.trackSpeed;
                settingsMain.prevInputOffsetMS = settingsMain.inputOffsetMS;
                settingsMain.prevAvOffsetMS = settingsMain.avOffsetMS;
                settingsMain.prevMissHighwayColor = settingsMain.missHighwayColor;
                settingsMain.prevMirrorMode = settingsMain.mirrorMode;
                settingsMain.fullscreenPrev = settingsMain.fullscreen;

                settingsMain.prevMainVolume = settingsMain.MainVolume;
                settingsMain.prevPlayerVolume = settingsMain.PlayerVolume;
                settingsMain.prevBandVolume = settingsMain.BandVolume;
                settingsMain.prevSFXVolume = settingsMain.SFXVolume;
                settingsMain.prevMissVolume = settingsMain.MissVolume;
                settingsMain.prevMenuVolume = settingsMain.MenuVolume;

                // player.InputOffset = settingsMain.inputOffsetMS / 1000.0f;
                // player.VideoOffset = settingsMain.avOffsetMS / 1000.0f;
                enctime.SetOffset(settingsMain.avOffsetMS / 1000.0);
                settingsMain.saveSettings(directory / "settings.json");

                TheMenuManager.SwitchScreen(MAIN_MENU);
            }
            static int selectedTab = 0;
            static int displayedTab = 0;

            static int selectedKbTab = 0;
            static int displayedKbTab = 0;

            GuiToggleGroup(
                { u.LeftSide + u.winpct(0.005f),
                  OvershellBottom,
                  (u.winpct(0.985f) / 5),
                  u.hinpct(0.05) },
                "Main;Highway;Volume;Keyboard Controls;Gamepad Controls",
                &selectedTab
            );
            if (!changingKey && !changingOverdrive && !changingPause) {
                displayedTab = selectedTab;
            } else {
                selectedTab = displayedTab;
            }
            if (!changingKey && !changingOverdrive && !changingPause) {
                displayedKbTab = selectedKbTab;
            } else {
                selectedKbTab = displayedKbTab;
            }
            float EntryFontSize = u.hinpct(0.03f);
            float EntryHeight = u.hinpct(0.05f);
            float EntryTop = OvershellBottom + u.hinpct(0.1f);
            float HeaderTextLeft = u.LeftSide + u.winpct(0.015f);
            float EntryTextLeft = u.LeftSide + u.winpct(0.025f);
            float EntryTextTop = EntryTop + u.hinpct(0.01f);
            float OptionLeft = u.LeftSide + u.winpct(0.005f) + u.winpct(0.989f) / 3;
            float OptionWidth = u.winpct(0.989f) / 3;
            float OptionRight = OptionLeft + OptionWidth;

            float underTabsHeight = OvershellBottom + u.hinpct(0.05f);

            GuiSetStyle(SLIDER, BASE_COLOR_NORMAL, 0x181827FF);
            GuiSetStyle(
                SLIDER,
                BASE_COLOR_PRESSED,
                ColorToInt(ColorBrightness(AccentColor, -0.25f))
            );
            GuiSetStyle(
                SLIDER,
                TEXT_COLOR_FOCUSED,
                ColorToInt(ColorBrightness(AccentColor, -0.5f))
            );
            GuiSetStyle(SLIDER, BORDER, 0xFFFFFFFF);
            GuiSetStyle(SLIDER, BORDER_COLOR_FOCUSED, 0xFFFFFFFF);
            GuiSetStyle(SLIDER, BORDER_WIDTH, 2);

            switch (displayedTab) {
            case MAIN: {
                // Main settings tab

                float trackSpeedFloat = settingsMain.trackSpeed;

                // header 1
                // calibration header
                int calibrationMenuOffset = 0;

                DrawRectangle(
                    u.wpct(0.005f),
                    underTabsHeight + (EntryHeight * calibrationMenuOffset),
                    OptionWidth * 2,
                    EntryHeight,
                    Color { 0, 0, 0, 128 }
                );
                DrawTextEx(
                    assets.rubikBoldItalic,
                    "Calibration",
                    { HeaderTextLeft,
                      OvershellBottom + u.hinpct(0.055f)
                          + (EntryHeight * calibrationMenuOffset) },
                    u.hinpct(0.04f),
                    0,
                    WHITE
                );

                // av offset

                settingsMain.avOffsetMS = sor.sliderEntry(
                    settingsMain.avOffsetMS,
                    -500.0f,
                    500.0f,
                    calibrationMenuOffset + 1,
                    "Audio/Visual Offset",
                    1
                );

                // input offset
                DrawRectangle(
                    u.wpct(0.005f),
                    underTabsHeight + (EntryHeight * (calibrationMenuOffset + 2)),
                    OptionWidth * 2,
                    EntryHeight,
                    Color { 0, 0, 0, 64 }
                );
                settingsMain.inputOffsetMS = sor.sliderEntry(
                    settingsMain.inputOffsetMS,
                    -500.0f,
                    500.0f,
                    calibrationMenuOffset + 2,
                    "Input Offset",
                    1
                );

                float calibrationTop =
                    EntryTop + (EntryHeight * (calibrationMenuOffset + 2));
                float calibrationTextTop =
                    EntryTextTop + (EntryHeight * (calibrationMenuOffset + 2));
                DrawTextEx(
                    assets.rubikBold,
                    "Automatic Calibration",
                    { EntryTextLeft, calibrationTextTop },
                    EntryFontSize,
                    0,
                    WHITE
                );
                if (GuiButton(
                        { OptionLeft, calibrationTop, OptionWidth, EntryHeight },
                        "Start Calibration"
                    )) {
                    TheMenuManager.SwitchScreen(CALIBRATION);
                }

                int generalOffset = 4;
                // general header
                DrawRectangle(
                    u.wpct(0.005f),
                    underTabsHeight + (EntryHeight * generalOffset),
                    OptionWidth * 2,
                    EntryHeight,
                    Color { 0, 0, 0, 128 }
                );
                DrawTextEx(
                    assets.rubikBoldItalic,
                    "General",
                    { HeaderTextLeft,
                      OvershellBottom + u.hinpct(0.055f)
                          + (EntryHeight * generalOffset) },
                    u.hinpct(0.04f),
                    0,
                    WHITE
                );

                // fullscreen

                settingsMain.fullscreen = sor.toggleEntry(
                    settingsMain.fullscreen, generalOffset + 1, "Fullscreen"
                );

                DrawRectangle(
                    u.wpct(0.005f),
                    underTabsHeight + (EntryHeight * (generalOffset + 2)),
                    OptionWidth * 2,
                    EntryHeight,
                    Color { 0, 0, 0, 64 }
                );

                float scanTop = EntryTop + (EntryHeight * (generalOffset + 1));
                float scanTextTop = EntryTextTop + (EntryHeight * (generalOffset + 1));
                DrawTextEx(
                    assets.rubikBold,
                    "Scan Songs",
                    { EntryTextLeft, scanTextTop },
                    EntryFontSize,
                    0,
                    WHITE
                );
                if (GuiButton({ OptionLeft, scanTop, OptionWidth, EntryHeight }, "Scan")) {
                    TheSongList.ScanSongs(settingsMain.songPaths);
                }

                break;
            }
            case HIGHWAY: {
                DrawRectangle(
                    u.wpct(0.005f),
                    OvershellBottom + u.hinpct(0.05f),
                    OptionWidth * 2,
                    EntryHeight,
                    Color { 0, 0, 0, 128 }
                );
                DrawTextEx(
                    assets.rubikBoldItalic,
                    "Highway",
                    { HeaderTextLeft, OvershellBottom + u.hinpct(0.055f) },
                    u.hinpct(0.04f),
                    0,
                    WHITE
                );
                settingsMain.trackSpeed = sor.sliderEntry(
                    settingsMain.trackSpeed,
                    0,
                    settingsMain.trackSpeedOptions.size() - 1,
                    1,
                    "Track Speed",
                    1.0f
                );
                // highway length

                DrawRectangle(
                    u.wpct(0.005f),
                    underTabsHeight + (EntryHeight * 2),
                    OptionWidth * 2,
                    EntryHeight,
                    Color { 0, 0, 0, 64 }
                );
                settingsMain.highwayLengthMult = sor.sliderEntry(
                    settingsMain.highwayLengthMult,
                    0.25f,
                    2.5f,
                    2,
                    "Highway Length Multiplier",
                    0.25f
                );

                // miss color
                settingsMain.missHighwayDefault = sor.toggleEntry(
                    settingsMain.missHighwayDefault, 3, "Highway Miss Color"
                );

                // lefty flip
                DrawRectangle(
                    u.wpct(0.005f),
                    underTabsHeight + (EntryHeight * 4),
                    OptionWidth * 2,
                    EntryHeight,
                    Color { 0, 0, 0, 64 }
                );
                settingsMain.mirrorMode =
                    sor.toggleEntry(settingsMain.mirrorMode, 4, "Mirror/Lefty Mode");

                // menu.hehe = sor.toggleEntry(menu.hehe, 5, "Super Cool Highway Colors");

                // TheGameRenderer.bot = sor.toggleEntry(TheGameRenderer.bot, 6, "Bot");

                // TheGameRenderer.showHitwindow = sor.toggleEntry(
                // TheGameRenderer.showHitwindow, 7, "Show Hitwindow");
                break;
            }
            case VOLUME: {
                // audio tab
                DrawRectangle(
                    u.wpct(0.005f),
                    OvershellBottom + u.hinpct(0.05f),
                    OptionWidth * 2,
                    EntryHeight,
                    Color { 0, 0, 0, 128 }
                );
                DrawTextEx(
                    assets.rubikBoldItalic,
                    "Volume",
                    { HeaderTextLeft, OvershellBottom + u.hinpct(0.055f) },
                    u.hinpct(0.04f),
                    0,
                    WHITE
                );

                settingsMain.MainVolume = sor.sliderEntry(
                    settingsMain.MainVolume, 0, 1, 1, "Main Volume", 0.05f
                );

                settingsMain.PlayerVolume = sor.sliderEntry(
                    settingsMain.PlayerVolume, 0, 1, 2, "Player Volume", 0.05f
                );

                settingsMain.BandVolume = sor.sliderEntry(
                    settingsMain.BandVolume, 0, 1, 3, "Band Volume", 0.05f
                );

                settingsMain.SFXVolume =
                    sor.sliderEntry(settingsMain.SFXVolume, 0, 1, 4, "SFX Volume", 0.05f);

                settingsMain.MissVolume = sor.sliderEntry(
                    settingsMain.MissVolume, 0, 1, 5, "Miss Volume", 0.05f
                );

                settingsMain.MenuVolume = sor.sliderEntry(
                    settingsMain.MenuVolume, 0, 1, 6, "Menu Music Volume", 0.05f
                );

                // player.selInstVolume = settingsMain.MainVolume *
                // settingsMain.PlayerVolume; player.otherInstVolume =
                // settingsMain.MainVolume * settingsMain.BandVolume; player.sfxVolume =
                // settingsMain.MainVolume * settingsMain.SFXVolume; player.missVolume =
                // settingsMain.MainVolume * settingsMain.MissVolume;

                break;
            }
            case KEYBOARD: {
                // Keyboard bindings tab
                GuiToggleGroup(
                    { u.LeftSide + u.winpct(0.005f),
                      OvershellBottom + u.hinpct(0.05f),
                      (u.winpct(0.987f) / 4),
                      u.hinpct(0.05) },
                    "Pad Binds;Classic Binds;Misc Gameplay;Menu Binds",
                    &selectedKbTab
                );
                GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
                switch (displayedKbTab) {
                case kbPAD: {
                    for (int i = 0; i < 5; i++) {
                        sor.keybindEntryText(i + 2, "Lane " + to_string(i + 1));
                        sor.keybind5kEntry(
                            settingsMain.keybinds5K[i],
                            i + 2,
                            "Lane " + to_string(i + 1),
                            keybinds,
                            i
                        );
                        sor.keybind5kAltEntry(
                            settingsMain.keybinds5KAlt[i],
                            i + 2,
                            "Lane " + to_string(i + 1),
                            keybinds,
                            i
                        );
                    }
                    for (int i = 0; i < 4; i++) {
                        sor.keybindEntryText(i + 8, "Lane " + to_string(i + 1));
                        sor.keybind4kEntry(
                            settingsMain.keybinds4K[i],
                            i + 8,
                            "Lane " + to_string(i + 1),
                            keybinds,
                            i
                        );
                        sor.keybind4kAltEntry(
                            settingsMain.keybinds4KAlt[i],
                            i + 8,
                            "Lane " + to_string(i + 1),
                            keybinds,
                            i
                        );
                    }
                    GuiSetStyle(DEFAULT, TEXT_SIZE, 28);
                    break;
                }
                case kbCLASSIC: {
                    DrawRectangle(
                        u.wpct(0.005f),
                        underTabsHeight + (EntryHeight * 2),
                        OptionWidth * 2,
                        EntryHeight,
                        ColorAlpha(GREEN, 0.1f)
                    );
                    DrawRectangle(
                        u.wpct(0.005f),
                        underTabsHeight + (EntryHeight * 3),
                        OptionWidth * 2,
                        EntryHeight,
                        ColorAlpha(RED, 0.1f)
                    );
                    DrawRectangle(
                        u.wpct(0.005f),
                        underTabsHeight + (EntryHeight * 4),
                        OptionWidth * 2,
                        EntryHeight,
                        ColorAlpha(YELLOW, 0.1f)
                    );
                    DrawRectangle(
                        u.wpct(0.005f),
                        underTabsHeight + (EntryHeight * 5),
                        OptionWidth * 2,
                        EntryHeight,
                        ColorAlpha(BLUE, 0.1f)
                    );
                    DrawRectangle(
                        u.wpct(0.005f),
                        underTabsHeight + (EntryHeight * 6),
                        OptionWidth * 2,
                        EntryHeight,
                        ColorAlpha(ORANGE, 0.1f)
                    );
                    for (int i = 0; i < 5; i++) {
                        sor.keybindEntryText(i + 2, "Lane " + to_string(i + 1));
                        sor.keybind5kEntry(
                            settingsMain.keybinds5K[i],
                            i + 2,
                            "Lane " + to_string(i + 1),
                            keybinds,
                            i
                        );
                        sor.keybind5kAltEntry(
                            settingsMain.keybinds5KAlt[i],
                            i + 2,
                            "Lane " + to_string(i + 1),
                            keybinds,
                            i
                        );
                    }
                    sor.keybindEntryText(8, "Strum Up");
                    sor.keybindStrumEntry(0, 8, settingsMain.keybindStrumUp, keybinds);

                    sor.keybindEntryText(9, "Strum Down");
                    sor.keybindStrumEntry(1, 9, settingsMain.keybindStrumDown, keybinds);

                    break;
                }
                case kbMISC: {
                    sor.keybindEntryText(2, "Overdrive");
                    sor.keybindOdAltEntry(
                        settingsMain.keybindOverdriveAlt, 2, "Overdrive Alt", keybinds
                    );
                    sor.keybindOdEntry(
                        settingsMain.keybindOverdrive, 2, "Overdrive", keybinds
                    );

                    sor.keybindEntryText(3, "Pause Song");
                    sor.keybindPauseEntry(settingsMain.keybindPause, 3, "Pause", keybinds);
                    break;
                }
                case kbMENUS: {
                    break;
                }
                }
                if (sor.changingKey) {
                    std::vector<int> &bindsToChange = sor.changingAlt
                        ? (sor.changing4k ? settingsMain.keybinds4KAlt
                                          : settingsMain.keybinds5KAlt)
                        : (sor.changing4k ? settingsMain.keybinds4K
                                          : settingsMain.keybinds5K);
                    DrawRectangle(
                        0, 0, GetScreenWidth(), GetScreenHeight(), { 0, 0, 0, 200 }
                    );
                    std::string keyString = (sor.changing4k ? "4k" : "5k");
                    std::string altString = (sor.changingAlt ? " alt" : "");
                    std::string changeString =
                        "Press a key for " + keyString + altString + " lane ";
                    GameMenu::mhDrawText(
                        assets.rubik,
                        changeString.c_str(),
                        { ((float)GetScreenWidth()) / 2,
                          (float)GetScreenHeight() / 2 - 30 },
                        20,
                        WHITE,
                        assets.sdfShader,
                        CENTER
                    );
                    int pressedKey = GetKeyPressed();
                    if (GuiButton(
                            { ((float)GetScreenWidth() / 2) - 130,
                              GetScreenHeight() - 60.0f,
                              120,
                              40 },
                            "Unbind Key"
                        )) {
                        pressedKey = -2;
                    }
                    if (GuiButton(
                            { ((float)GetScreenWidth() / 2) + 10,
                              GetScreenHeight() - 60.0f,
                              120,
                              40 },
                            "Cancel"
                        )) {
                        sor.selLane = 0;
                        sor.changingKey = false;
                    }
                    if (pressedKey != 0) {
                        bindsToChange[sor.selLane] = pressedKey;
                        sor.selLane = 0;
                        sor.changingKey = false;
                    }
                }
                if (sor.changingOverdrive) {
                    DrawRectangle(
                        0, 0, GetScreenWidth(), GetScreenHeight(), { 0, 0, 0, 200 }
                    );
                    std::string altString = (sor.changingAlt ? " alt" : "");
                    std::string changeString =
                        "Press a key for " + altString + " overdrive";
                    GameMenu::mhDrawText(
                        assets.rubik,
                        changeString.c_str(),
                        { ((float)GetScreenWidth()) / 2,
                          (float)GetScreenHeight() / 2 - 30 },
                        20,
                        WHITE,
                        assets.sdfShader,
                        CENTER
                    );
                    int pressedKey = GetKeyPressed();
                    if (GuiButton(
                            { ((float)GetScreenWidth() / 2) - 130,
                              GetScreenHeight() - 60.0f,
                              120,
                              40 },
                            "Unbind Key"
                        )) {
                        pressedKey = -2;
                    }
                    if (GuiButton(
                            { ((float)GetScreenWidth() / 2) + 10,
                              GetScreenHeight() - 60.0f,
                              120,
                              40 },
                            "Cancel"
                        )) {
                        sor.changingAlt = false;
                        sor.changingOverdrive = false;
                    }
                    if (pressedKey != 0) {
                        if (sor.changingAlt)
                            settingsMain.keybindOverdriveAlt = pressedKey;
                        else
                            settingsMain.keybindOverdrive = pressedKey;
                        sor.changingOverdrive = false;
                    }
                }
                if (sor.changingPause) {
                    DrawRectangle(
                        0, 0, GetScreenWidth(), GetScreenHeight(), { 0, 0, 0, 200 }
                    );
                    GameMenu::mhDrawText(
                        assets.rubik,
                        "Press a key for Pause",
                        { ((float)GetScreenWidth()) / 2,
                          (float)GetScreenHeight() / 2 - 30 },
                        20,
                        WHITE,
                        assets.sdfShader,
                        CENTER
                    );
                    int pressedKey = GetKeyPressed();
                    if (GuiButton(
                            { ((float)GetScreenWidth() / 2) - 130,
                              GetScreenHeight() - 60.0f,
                              120,
                              40 },
                            "Unbind Key"
                        )) {
                        pressedKey = -2;
                    }
                    if (GuiButton(
                            { ((float)GetScreenWidth() / 2) + 10,
                              GetScreenHeight() - 60.0f,
                              120,
                              40 },
                            "Cancel"
                        )) {
                        sor.changingAlt = false;
                        sor.changingPause = false;
                    }
                    if (pressedKey != 0) {
                        settingsMain.keybindPause = pressedKey;
                        sor.changingPause = false;
                    }
                }
                if (sor.changingStrumUp) {
                    DrawRectangle(
                        0, 0, GetScreenWidth(), GetScreenHeight(), { 0, 0, 0, 200 }
                    );
                    GameMenu::mhDrawText(
                        assets.rubik,
                        "Press a key for Strum Up",
                        { ((float)GetScreenWidth()) / 2,
                          (float)GetScreenHeight() / 2 - 30 },
                        20,
                        WHITE,
                        assets.sdfShader,
                        CENTER
                    );
                    int pressedKey = GetKeyPressed();
                    if (GuiButton(
                            { ((float)GetScreenWidth() / 2) - 130,
                              GetScreenHeight() - 60.0f,
                              120,
                              40 },
                            "Unbind Key"
                        )) {
                        pressedKey = -2;
                    }
                    if (GuiButton(
                            { ((float)GetScreenWidth() / 2) + 10,
                              GetScreenHeight() - 60.0f,
                              120,
                              40 },
                            "Cancel"
                        )) {
                        sor.changingAlt = false;
                        sor.changingStrumUp = false;
                    }
                    if (pressedKey != 0) {
                        settingsMain.keybindStrumUp = pressedKey;
                        sor.changingStrumUp = false;
                    }
                }
                if (sor.changingStrumDown) {
                    DrawRectangle(
                        0, 0, GetScreenWidth(), GetScreenHeight(), { 0, 0, 0, 200 }
                    );
                    GameMenu::mhDrawText(
                        assets.rubik,
                        "Press a key for Strum Down",
                        { ((float)GetScreenWidth()) / 2,
                          (float)GetScreenHeight() / 2 - 30 },
                        20,
                        WHITE,
                        assets.sdfShader,
                        CENTER
                    );
                    int pressedKey = GetKeyPressed();
                    if (GuiButton(
                            { ((float)GetScreenWidth() / 2) - 130,
                              GetScreenHeight() - 60.0f,
                              120,
                              40 },
                            "Unbind Key"
                        )) {
                        pressedKey = -2;
                    }
                    if (GuiButton(
                            { ((float)GetScreenWidth() / 2) + 10,
                              GetScreenHeight() - 60.0f,
                              120,
                              40 },
                            "Cancel"
                        )) {
                        sor.changingAlt = false;
                        sor.changingStrumDown = false;
                    }
                    if (pressedKey != 0) {
                        settingsMain.keybindStrumDown = pressedKey;
                        sor.changingStrumDown = false;
                    }
                }
                break;
            }
            case GAMEPAD: { /*
                 //Controller bindings tab
                 GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
                 for (int i = 0; i < 5; i++) {
                     float j = (float) i - 2.0f;
                     if (GuiButton({
                                     ((float) GetScreenWidth() / 2) - 40 + (
                                         80 * j),
                                     240, 80, 60
                                 },
                                 keybinds.getControllerStr(
                                     controllerID,
                                     settingsMain.controller5K[i],
                                     settingsMain.controllerType,
                                     settingsMain.controller5KAxisDirection[
                                         i]).c_str())) {
                         changing4k = false;
                         selLane = i;
                         changingKey = true;
                     }
                 }
                 for (int i = 0; i < 4; i++) {
                     float j = (float) i - 1.5f;
                     if (GuiButton({
                                     ((float) GetScreenWidth() / 2) - 40 + (
                                         80 * j),
                                     360, 80, 60
                                 },
                                 keybinds.getControllerStr(
                                     controllerID,
                                     settingsMain.controller4K[i],
                                     settingsMain.controllerType,
                                     settingsMain.controller4KAxisDirection[
                                         i]).c_str())) {
                         changing4k = true;
                         selLane = i;
                         changingKey = true;
                     }
                 }
                 if (GuiButton({((float) GetScreenWidth() / 2) - 40, 480, 80, 60},
                             keybinds.getControllerStr(
                                 controllerID, settingsMain.controllerOverdrive,
                                 settingsMain.controllerType,
                                 settingsMain.controllerOverdriveAxisDirection).
                             c_str())) {
                     changingKey = false;
                     changingOverdrive = true;
                 }
                 if (GuiButton({((float) GetScreenWidth() / 2) - 40, 560, 80, 60},
                             keybinds.getControllerStr(
                                 controllerID, settingsMain.controllerPause,
                                 settingsMain.controllerType,
                                 settingsMain.controllerPauseAxisDirection).
                             c_str())) {
                     changingKey = false;
                     changingOverdrive = true;
                 }
                 if (changingKey) {
                     std::vector<int> &bindsToChange = (changing4k
                                                             ? settingsMain.
                                                             controller4K
                                                             : settingsMain.
                                                             controller5K);
                     std::vector<int> &directionToChange = (changing4k
                                                                 ? settingsMain.
                                                                 controller4KAxisDirection
                                                                 : settingsMain.
                                                                 controller5KAxisDirection);
                     DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                                 {0, 0, 0, 200});
                     std::string keyString = (changing4k ? "4k" : "5k");
                     std::string changeString =
                             "Press a button/axis for controller " +
                             keyString + " lane " +
                             std::to_string(selLane + 1);
                     DrawTextRubik(changeString.c_str(),
                                 ((float) GetScreenWidth() - MeasureTextRubik(
                                     changeString.c_str(), 20)) / 2,
                                 GetScreenHeight() / 2 - 30, 20, WHITE);
                     if (GuiButton({
                                     ((float) GetScreenWidth() / 2) - 60,
                                     GetScreenHeight() - 60.0f, 120, 40
                                 },
                                 "Cancel")) {
                         changingKey = false;
                     }
                     if (pressedGamepadInput != -999) {
                         bindsToChange[selLane] = pressedGamepadInput;
                         if (pressedGamepadInput < 0) {
                             directionToChange[selLane] = axisDirection;
                         }
                         selLane = 0;
                         changingKey = false;
                         pressedGamepadInput = -999;
                     }
                 }
                 if (changingOverdrive) {
                     DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                                 {0, 0, 0, 200});
                     std::string changeString =
                             "Press a button/axis for controller overdrive";
                     DrawTextRubik(changeString.c_str(),
                                 ((float) GetScreenWidth() - MeasureTextRubik(
                                     changeString.c_str(), 20)) / 2,
                                 GetScreenHeight() / 2 - 30, 20, WHITE);
                     if (GuiButton({
                                     ((float) GetScreenWidth() / 2) - 60,
                                     GetScreenHeight() - 60.0f, 120, 40
                                 },
                                 "Cancel")) {
                         changingOverdrive = false;
                     }
                     if (pressedGamepadInput != -999) {
                         settingsMain.controllerOverdrive = pressedGamepadInput;
                         if (pressedGamepadInput < 0) {
                             settingsMain.controllerOverdriveAxisDirection =
                                     axisDirection;
                         }
                         changingOverdrive = false;
                         pressedGamepadInput = -999;
                     }
                 }
                 if (changingPause) {
                     DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                                 {0, 0, 0, 200});
                     std::string changeString =
                             "Press a button/axis for controller pause";
                     DrawTextRubik(changeString.c_str(),
                                 ((float) GetScreenWidth() - MeasureTextRubik(
                                     changeString.c_str(), 20)) / 2,
                                 GetScreenHeight() / 2 - 30, 20, WHITE);
                     if (GuiButton({
                                     ((float) GetScreenWidth() / 2) - 60,
                                     GetScreenHeight() - 60.0f, 120, 40
                                 },
                                 "Cancel")) {
                         changingPause = false;
                     }
                     if (pressedGamepadInput != -999) {
                         settingsMain.controllerPause = pressedGamepadInput;
                         if (pressedGamepadInput < 0) {
                             settingsMain.controllerPauseAxisDirection =
                                     axisDirection;
                         }
                         changingPause = false;
                         pressedGamepadInput = -999;
                     }
                 }
                 GuiSetStyle(DEFAULT, TEXT_SIZE, 28);
                 break;*/
            }
            }
            break;
        }
        case SONG_SELECT: {
            ActiveMenu->Draw();
            break;
        }
        case READY_UP: {
            ActiveMenu->Draw();
            break;
        }
        case GAMEPLAY: {
            ActiveMenu->Draw();
            break;
        }
        case RESULTS: {
            ActiveMenu->Draw();
            break;
        }
        case CHART_LOADING_SCREEN: {
            ClearBackground(BLACK);
            if (StartLoading) {
                TheSongList.curSong->LoadAlbumArt();
                std::thread ChartLoader(LoadCharts);
                ChartLoader.detach();
                StartLoading = false;
            }
            GameMenu::DrawAlbumArtBackground(TheSongList.curSong->albumArtBlur);
            encOS::DrawTopOvershell(0.15f);
            DrawTextEx(
                assets.redHatDisplayBlack,
                "LOADING...  ",
                { u.LeftSide, u.hpct(0.05f) },
                u.hinpct(0.125f),
                0,
                WHITE
            );
            float AfterLoadingTextPos =
                MeasureTextEx(
                    assets.redHatDisplayBlack, "LOADING...  ", u.hinpct(0.125f), 0
                )
                    .x;

            std::string LoadingPhrase = "";

            switch (LoadingState) {
            case BEATLINES: {
                LoadingPhrase = "Setting metronome";
                break;
            }
            case NOTE_PARSING: {
                LoadingPhrase = "Loading notes";
                break;
            }
            case NOTE_SORTING: {
                LoadingPhrase = "Cleaning up notes";
                break;
            }
            case PLASTIC_CALC: {
                LoadingPhrase = "Fixing up Classic";
                break;
            }
            case NOTE_MODIFIERS: {
                LoadingPhrase = "HOPO on, HOPO off";
                break;
            }
            case OVERDRIVE: {
                LoadingPhrase = "Gettin' your double points on";
                break;
            }
            case SOLOS: {
                LoadingPhrase = "Shuffling through solos";
                break;
            }
            case BASE_SCORE: {
                LoadingPhrase = "Calculating stars";
                break;
            }
            case EXTRA_PROCESSING: {
                LoadingPhrase = "Finishing touch-ups";
                break;
            }
            case READY: {
                LoadingPhrase = "Ready!";
                break;
            }
            default: {
                LoadingPhrase = "";
                break;
            }
            }

            DrawTextEx(
                assets.rubikBold,
                LoadingPhrase.c_str(),
                { u.LeftSide + AfterLoadingTextPos + u.winpct(0.02f), u.hpct(0.09f) },
                u.hinpct(0.05f),
                0,
                LIGHTGRAY
            );
            GameMenu::DrawBottomOvershell();

            if (FinishedLoading) {
                TheGameRenderer.LoadGameplayAssets();
                FinishedLoading = false;
                StartLoading = true;
                TheMenuManager.SwitchScreen(GAMEPLAY);
            }
            break;
        }
        case SOUND_TEST: {
            ActiveMenu->Draw();
            break;
        }
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
