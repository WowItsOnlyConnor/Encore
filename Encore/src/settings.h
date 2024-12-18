//
// Created by marie on 02/10/2024.
//

#ifndef SETTINGS_H
#define SETTINGS_H
#include <array>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <nlohmann/json.hpp>

#define SETTINGS_OPTIONS                                                                 \
    OPTION(float, MainVolume, 0.5f)                                                      \
    OPTION(float, ActiveInstrumentVolume, 0.75f)                                         \
    OPTION(float, InactiveInstrumentVolume, 0.5f)                                        \
    OPTION(float, SoundEffectVolume, 0.5f)                                               \
    OPTION(float, MuteVolume, 0.15f)                                                     \
    OPTION(float, MenuMusicVolume, 0.15f)                                                \
    OPTION(bool, Fullscreen, false)                                                      \
    OPTION(int, AudioOffset, 0)                                                          \
    OPTION(bool, DiscordRichPresence, true)

namespace Encore {
    class Settings {
    public:
#define OPTION(type, value, default) type value = default;
        SETTINGS_OPTIONS;
#undef OPTION
        std::vector<std::filesystem::path> SongPaths;
    };

    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
        Settings,
        MainVolume,
        ActiveInstrumentVolume,
        InactiveInstrumentVolume,
        SoundEffectVolume,
        MuteVolume,
        MenuMusicVolume,
        Fullscreen,
        AudioOffset,
        DiscordRichPresence,
        SongPaths
    );

};

extern Encore::Settings TheGameSettings;

#endif // SETTINGS_H
