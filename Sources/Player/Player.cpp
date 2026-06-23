#include "cheats.hpp"
#include "MH4G/Player.hpp"

#include <iomanip>
#include <sstream>
#include <vector>

namespace CTRPluginFramework
{
    namespace
    {
        struct PlayerListItem
        {
            u32 slot;
            volatile MH4G::UPlayer *player;
        };

        std::string FormatPlayerLabel(u32 slot, volatile MH4G::UPlayer *player)
        {
            std::ostringstream output;
            output << "Player " << (slot + 1);
            if (slot == 0)
                output << " (Local)";
            output << " [0x" << std::hex << std::uppercase
                   << reinterpret_cast<u32>(player) << "]";
            return output.str();
        }

        std::string FormatPlayerInfo(const PlayerListItem &item)
        {
            volatile MH4G::BasePlayerWork *work = item.player->GetBaseWork();
            if (work == nullptr)
                return "The selected player has no base player work.";

            std::ostringstream output;
            output << std::fixed << std::setprecision(2);
            output << "Slot: " << (item.slot + 1);
            if (item.slot == 0)
                output << " (Local)";
            output << "\nPlayer: 0x" << std::hex << std::uppercase
                   << reinterpret_cast<u32>(item.player);
            output << "\nBase work: 0x" << reinterpret_cast<u32>(work);
            output << std::dec;
            output << "\n\nPosition"
                   << "\n  X: " << work->positionX
                   << "\n  Y: " << work->positionY
                   << "\n  Z: " << work->positionZ;
            output << "\n\nHealth: " << work->health
                   << " / " << work->maximumHealth;
            output << "\nStamina: " << work->stamina
                   << " / " << work->maximumStamina;
            output << "\nSpeed: " << work->playerSpeed;
            output << "\nRoom: " << work->roomNumber;
            output << "\nPlayer type: " << static_cast<u32>(work->playerType);
            output << "\nCharacter type: " << static_cast<u32>(work->characterType);
            output << "\nPlayer state: " << static_cast<u32>(work->playerState);
            output << "\nAction state: " << static_cast<u32>(work->actionState);
            output << "\nAction number: " << work->actionNumber;
            output << "\nMove/Draw: " << static_cast<u32>(work->moveEnabled)
                   << " / " << static_cast<u32>(work->drawEnabled);
            return output.str();
        }
    }

    void getPlayerInfo()
    {
        volatile MH4G::SPlayer *manager = MH4G::SPlayer::GetInstance();
        if (manager == nullptr)
        {
            MessageBox("Player Information", "The player manager is not available.")();
            return;
        }

        std::vector<PlayerListItem> players;
        StringVector options;

        for (u32 slot = 0; slot < 4; ++slot)
        {
            volatile MH4G::UPlayer *player = manager->GetPlayerSlot(slot);
            if (player == nullptr)
                continue;

            players.push_back({slot, player});
            options.push_back(FormatPlayerLabel(slot, player));
        }

        if (players.empty())
        {
            MessageBox("Player Information", "No players are currently available.")();
            return;
        }

        Keyboard keyboard("Select a player");
        keyboard.Populate(options);
        int selected = keyboard.Open();
        if (selected < 0 || static_cast<size_t>(selected) >= players.size())
            return;

        MessageBox("Player Information", FormatPlayerInfo(players[selected]))();
    }
}
