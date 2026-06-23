#include "MH4G/Logger.hpp"
#include "MH4G/Player.hpp"
#include <CTRPluginFramework.hpp>

#include <algorithm>

namespace CTRPluginFramework
{
    namespace
    {
        const char *LogDirectory = "/MH4G-Plugin";
        const char *LogPath = "/MH4G-Plugin/chat.log";
        MenuEntry *g_autoDumpEntry = nullptr;

        std::string SanitizeLogField(std::string value)
        {
            std::replace(value.begin(), value.end(), '\r', ' ');
            std::replace(value.begin(), value.end(), '\n', ' ');
            return value;
        }

        bool ReadPlayerName(u32 slot, std::string &name)
        {
            volatile MH4G::SPlayer *manager = MH4G::SPlayer::GetInstance();
            if (manager == nullptr || slot >= 4)
                return false;

            volatile MH4G::UPlayer *player = manager->GetPlayerSlot(slot);
            if (player == nullptr)
                return false;

            volatile MH4G::BasePlayerWork *work = player->GetBaseWork();
            if (work == nullptr || work->playerDataAddress == 0)
                return false;

            const u32 nameAddress = work->playerDataAddress + 0x18;
            if (!Process::CheckAddress(nameAddress, MEMPERM_READ))
                return false;

            return Process::ReadString(nameAddress, name, 0x1A, StringFormat::Utf16);
        }

        bool AppendChatLine(const std::string &playerName, const std::string &chatContent)
        {
            Directory::Create(LogDirectory);

            File log;
            int result = File::Open(log, LogPath,
                                    File::WRITE | File::CREATE | File::APPEND | File::SYNC);
            if (result != File::SUCCESS)
                return false;

            const std::string line = "[" + SanitizeLogField(playerName) + "] " +
                                     SanitizeLogField(chatContent);
            return log.WriteLine(line) == File::SUCCESS;
        }
    }

    void UpdateChatAutoDump(MenuEntry *entry)
    {
        // Keeping the entry lets receive hooks query its current state even
        // after CTRPF stops executing the game callback on deactivation.
        g_autoDumpEntry = entry;
    }

    bool IsChatAutoDumpEnabled()
    {
        return g_autoDumpEntry != nullptr && g_autoDumpEntry->IsActivated();
    }

    bool DumpChatMessage(const std::string &playerName, const std::string &chatContent)
    {
        if (!IsChatAutoDumpEnabled() || playerName.empty() || chatContent.empty())
            return false;

        return AppendChatLine(playerName, chatContent);
    }

    bool DumpChatMessage(u32 playerSlot, const std::string &chatContent)
    {
        if (!IsChatAutoDumpEnabled())
            return false;

        std::string playerName;
        if (!ReadPlayerName(playerSlot, playerName))
            playerName = "Player " + std::to_string(playerSlot + 1);

        return DumpChatMessage(playerName, chatContent);
    }

    bool DumpChatMessageUtf16(u32 playerSlot, u32 chatAddress, u32 byteLength)
    {
        if (!IsChatAutoDumpEnabled() || chatAddress == 0 || byteLength == 0 ||
            !Process::CheckAddress(chatAddress, MEMPERM_READ))
            return false;

        std::string chatContent;
        if (!Process::ReadString(chatAddress, chatContent, byteLength, StringFormat::Utf16))
            return false;

        return DumpChatMessage(playerSlot, chatContent);
    }
}
