#include "MH4G/OverlayMenu.hpp"
#include "MH4G/GUI.hpp"
#include "MH4G/Player.hpp"

#include <CTRPluginFramework.hpp>

#include <cstdio>
#include <string>

namespace CTRPluginFramework
{
    namespace
    {
        enum class OverlayPage
        {
            Root,
            Player,
            PlayerInfo,
            PlayerDetail
        };

        bool g_running = false;
        bool g_open = false;
        OverlayPage g_page = OverlayPage::Root;
        u32 g_cursor = 0;
        u32 g_selectedPlayer = 0;
        MH4G::GUI::CDraw *g_draw = nullptr;

        std::string Hex(u32 value)
        {
            char buffer[16];
            std::snprintf(buffer, sizeof(buffer), "0x%08lX", static_cast<unsigned long>(value));
            return std::string(buffer);
        }

        void EnsureGameDraw()
        {
            if (g_draw != nullptr)
                return;

            g_draw = MH4G::GUI::CDraw::Create();
        }

        u32 GetItemCount()
        {
            switch (g_page)
            {
            case OverlayPage::Root:
            case OverlayPage::Player:
                return 1;
            case OverlayPage::PlayerInfo:
                return 4;
            case OverlayPage::PlayerDetail:
                return 0;
            }

            return 0;
        }

        std::string GetTitle()
        {
            switch (g_page)
            {
            case OverlayPage::Root:
                return "オーバーレイ";
            case OverlayPage::Player:
                return "オーバーレイ / プレイヤー";
            case OverlayPage::PlayerInfo:
                return "オーバーレイ / プレイヤー / プレイヤー情報";
            case OverlayPage::PlayerDetail:
                return "プレイヤー情報" + std::to_string(g_selectedPlayer + 1);
            }

            return "Overlay";
        }

        std::string GetItemLabel(u32 index)
        {
            switch (g_page)
            {
            case OverlayPage::Root:
                return index == 0 ? "プレイヤー" : "";
            case OverlayPage::Player:
                return index == 0 ? "プレイヤー情報" : "";
            case OverlayPage::PlayerInfo:
                return "プレイヤー情報" + std::to_string(index + 1);
            case OverlayPage::PlayerDetail:
                return "";
            }

            return "";
        }

        void EnterSelection()
        {
            switch (g_page)
            {
            case OverlayPage::Root:
                g_page = OverlayPage::Player;
                g_cursor = 0;
                break;
            case OverlayPage::Player:
                g_page = OverlayPage::PlayerInfo;
                g_cursor = 0;
                break;
            case OverlayPage::PlayerInfo:
                g_selectedPlayer = g_cursor;
                g_page = OverlayPage::PlayerDetail;
                g_cursor = 0;
                break;
            case OverlayPage::PlayerDetail:
                break;
            }
        }

        void GoBack()
        {
            switch (g_page)
            {
            case OverlayPage::Root:
                g_open = false;
                break;
            case OverlayPage::Player:
                g_page = OverlayPage::Root;
                g_cursor = 0;
                break;
            case OverlayPage::PlayerInfo:
                g_page = OverlayPage::Player;
                g_cursor = 0;
                break;
            case OverlayPage::PlayerDetail:
                g_page = OverlayPage::PlayerInfo;
                g_cursor = g_selectedPlayer;
                break;
            }
        }

        void UpdateInput()
        {
            if (Controller::IsKeyPressed(Start))
            {
                g_open = !g_open;
                if (g_open)
                {
                    g_page = OverlayPage::Root;
                    g_cursor = 0;
                    EnsureGameDraw();
                }
                return;
            }

            if (!g_open)
                return;

            if (Controller::IsKeyPressed(B))
            {
                GoBack();
                return;
            }

            const u32 itemCount = GetItemCount();
            if (itemCount != 0)
            {
                if (Controller::IsKeyPressed(Up))
                    g_cursor = (g_cursor + itemCount - 1) % itemCount;
                else if (Controller::IsKeyPressed(Down))
                    g_cursor = (g_cursor + 1) % itemCount;

                if (Controller::IsKeyPressed(A))
                    EnterSelection();
            }
        }

        void DrawHeader(const Screen &screen, const Color &accent)
        {
            screen.DrawRect(20, 14, 360, 28, accent);
            screen.DrawSysfont(GetTitle(), 30, 19, Color::Black);
            screen.DrawRect(20, 14, 360, 212, Color::White, false);
        }

        void DrawHelp(const Screen &screen)
        {
            screen.Draw("START Close   A Select   B Back", 34, 211,
                        Color(190, 198, 210, 255), Color(0, 0, 0, 0));
        }

        void DrawDirectory(const Screen &screen)
        {
            const Color accent = Color(245, 202, 92, 255);
            const Color panel = Color(20, 24, 30, 225);
            const Color selected = Color(245, 202, 92, 255);
            const Color row = Color(38, 44, 54, 230);

            screen.DrawRect(20, 14, 360, 212, panel);
            DrawHeader(screen, accent);

            const u32 itemCount = GetItemCount();
            for (u32 i = 0; i < itemCount; ++i)
            {
                const u32 y = 60 + i * 30;
                const bool active = i == g_cursor;
                screen.DrawRect(36, y, 328, 24, active ? selected : row);
                screen.DrawRect(36, y, 328, 24, active ? Color::White : Color(84, 96, 110, 255), false);

                const std::string prefix = g_page == OverlayPage::PlayerInfo ? "  " : "> ";
                screen.DrawSysfont(prefix + GetItemLabel(i), 48, y + 3,
                                   active ? Color::Black : Color::White);
            }

            const u32 typeInfo = g_draw == nullptr ? 0 : MH4G::GUI::CDraw::GetTypeInfo();
            screen.Draw("Game draw: " + (g_draw == nullptr ? std::string("none") : Hex(reinterpret_cast<u32>(g_draw))),
                        34, 184, Color(190, 198, 210, 255), Color(0, 0, 0, 0));
            screen.Draw("Type: " + Hex(typeInfo), 232, 184,
                        Color(190, 198, 210, 255), Color(0, 0, 0, 0));
            DrawHelp(screen);
        }

        void DrawStat(const Screen &screen, u32 x, u32 y, const std::string &label,
                      const std::string &value)
        {
            screen.Draw(label, x, y, Color(176, 186, 198, 255), Color(0, 0, 0, 0));
            screen.Draw(value, x + 94, y, Color::White, Color(0, 0, 0, 0));
        }

        void DrawPlayerDetail(const Screen &screen)
        {
            const Color accent = Color(94, 206, 170, 255);
            const Color panel = Color(20, 24, 30, 225);

            screen.DrawRect(20, 14, 360, 212, panel);
            DrawHeader(screen, accent);

            volatile MH4G::SPlayer *manager = MH4G::SPlayer::GetInstance();
            volatile MH4G::UPlayer *player = nullptr;
            volatile MH4G::BasePlayerWork *work = nullptr;

            if (manager != nullptr)
                player = manager->GetPlayerSlot(g_selectedPlayer);
            if (player != nullptr)
                work = player->GetBaseWork();

            if (manager == nullptr || player == nullptr || work == nullptr)
            {
                screen.DrawSysfont("Player data is not available.", 42, 72, Color::White);
                DrawHelp(screen);
                return;
            }

            char buffer[64];
            std::snprintf(buffer, sizeof(buffer), "%.2f, %.2f, %.2f",
                          work->positionX, work->positionY, work->positionZ);

            DrawStat(screen, 42, 62, "Player", Hex(reinterpret_cast<u32>(player)));
            DrawStat(screen, 42, 78, "Base work", Hex(reinterpret_cast<u32>(work)));
            DrawStat(screen, 42, 94, "Position", buffer);

            std::snprintf(buffer, sizeof(buffer), "%u / %u",
                          static_cast<unsigned int>(work->health),
                          static_cast<unsigned int>(work->maximumHealth));
            DrawStat(screen, 42, 110, "Health", buffer);

            std::snprintf(buffer, sizeof(buffer), "%.2f / %.2f",
                          work->stamina, work->maximumStamina);
            DrawStat(screen, 42, 126, "Stamina", buffer);

            std::snprintf(buffer, sizeof(buffer), "%.2f", work->playerSpeed);
            DrawStat(screen, 42, 142, "Speed", buffer);

            std::snprintf(buffer, sizeof(buffer), "%ld", static_cast<long>(work->roomNumber));
            DrawStat(screen, 42, 158, "Room", buffer);

            std::snprintf(buffer, sizeof(buffer), "%u / %u / %u",
                          static_cast<unsigned int>(work->playerState),
                          static_cast<unsigned int>(work->actionState),
                          static_cast<unsigned int>(work->actionNumber));
            DrawStat(screen, 42, 174, "State", buffer);

            std::snprintf(buffer, sizeof(buffer), "%u / %u",
                          static_cast<unsigned int>(work->moveEnabled),
                          static_cast<unsigned int>(work->drawEnabled));
            DrawStat(screen, 42, 190, "Move/Draw", buffer);

            DrawHelp(screen);
        }

        bool OverlayCallback(const Screen &screen)
        {
            if (!screen.IsTop)
                return false;

            UpdateInput();

            if (!g_open)
                return false;

            if (g_page == OverlayPage::PlayerDetail)
                DrawPlayerDetail(screen);
            else
                DrawDirectory(screen);

            return true;
        }
    }

    void StartOverlayMenu()
    {
        if (g_running)
            return;

        OSD::Run(OverlayCallback);
        g_running = true;
    }

    void StopOverlayMenu()
    {
        if (!g_running)
            return;

        OSD::Stop(OverlayCallback);
        g_running = false;
        g_open = false;
        g_draw = nullptr;
    }
}
