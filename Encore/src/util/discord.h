//
// Created by maria on 16/12/2024.
//

#ifndef DISCORD_H
#define DISCORD_H
#include <string>

namespace Encore {
    void DiscordInitialize();
    void DiscordShutdown();
    void DiscordUpdatePresence(
        const std::string &title, const std::string &details
    );
    void DiscordUpdatePresenceSong(const std::string &title, const std::string &details, int instrument);
};



#endif //DISCORD_H
