#include "MH4G/OverlayMenu.hpp"
#include "MH4G/GameKeyboard.hpp"
#include "MH4G/GUI.hpp"
#include "MH4G/Player.hpp"

#include <CTRPluginFramework.hpp>

#include <cstddef>
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
            PlayerDetail,
            Keyboard
        };

        namespace Layout
        {
            constexpr u32 PanelX = 8;
            constexpr u32 PanelY = 12;
            constexpr u32 PanelW = 214;
            constexpr u32 PanelH = 216;
            constexpr u32 HeaderH = 26;
            constexpr u32 ListX = PanelX + 12;
            constexpr u32 ListY = PanelY + 46;
            constexpr u32 RowW = PanelW - 24;
            constexpr u32 RowH = 24;
            constexpr u32 RowGap = 4;
            constexpr u32 RowStep = RowH + RowGap;
            constexpr u32 VisibleRows = 5;
            constexpr u32 HelpY = PanelY + PanelH - 16;
        }

        struct MenuItem
        {
            const char *label;
            OverlayPage target;
        };

        struct MenuPage
        {
            OverlayPage key;
            const char *title;
            OverlayPage back;
            const MenuItem *items;
            u32 itemCount;
        };

        template <size_t Size>
        u32 CountOf(const MenuItem (&)[Size])
        {
            return static_cast<u32>(Size);
        }

#define MENU_ITEMS(name) static const MenuItem name[]
#define MENU_PAGE(key, title, back, items) { key, title, back, items, CountOf(items) }

        MENU_ITEMS(RootItems) = {
            {"Keyboard", OverlayPage::Keyboard},
            {"プレイヤー", OverlayPage::Player},
        };

        MENU_ITEMS(PlayerItems) = {
            {"プレイヤー情報", OverlayPage::PlayerInfo},
        };

        // Dictionary-style page table. To add a static menu page, add:
        // 1. an OverlayPage value, 2. a MENU_ITEMS block, 3. a MENU_PAGE row.
        static const MenuPage MenuPages[] = {
            MENU_PAGE(OverlayPage::Root, "オーバーレイ", OverlayPage::Root, RootItems),
            MENU_PAGE(OverlayPage::Player, "オーバーレイ / プレイヤー", OverlayPage::Root, PlayerItems),
        };

        bool g_running = false;
        bool g_open = false;
        OverlayPage g_page = OverlayPage::Root;
        u32 g_cursor = 0;
        u32 g_scroll = 0;
        u32 g_selectedPlayer = 0;
        std::string g_keyboardResult;
        std::string g_keyboardStatus = "A: open keyboard";
        u16 g_keyboardRaw[4] = {};
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

        const MenuPage *FindMenuPage(OverlayPage page)
        {
            for (u32 i = 0; i < sizeof(MenuPages) / sizeof(MenuPages[0]); ++i)
            {
                if (MenuPages[i].key == page)
                    return &MenuPages[i];
            }

            return nullptr;
        }

        u32 GetItemCount()
        {
            if (g_page == OverlayPage::PlayerInfo)
                return 4;
            if (g_page == OverlayPage::Keyboard)
                return 1;

            const MenuPage *page = FindMenuPage(g_page);
            return page == nullptr ? 0 : page->itemCount;
        }

        void ResetCursor()
        {
            g_cursor = 0;
            g_scroll = 0;
        }

        void EnsureCursorVisible()
        {
            const u32 itemCount = GetItemCount();
            if (itemCount == 0)
            {
                g_cursor = 0;
                g_scroll = 0;
                return;
            }

            if (g_cursor >= itemCount)
                g_cursor = itemCount - 1;

            if (g_cursor < g_scroll)
                g_scroll = g_cursor;
            else if (g_cursor >= g_scroll + Layout::VisibleRows)
                g_scroll = g_cursor - Layout::VisibleRows + 1;

            const u32 maxScroll = itemCount > Layout::VisibleRows ? itemCount - Layout::VisibleRows : 0;
            if (g_scroll > maxScroll)
                g_scroll = maxScroll;
        }

        std::string GetTitle()
        {
            if (g_page == OverlayPage::Keyboard)
                return "Game Keyboard";
            if (g_page == OverlayPage::PlayerInfo)
                return "オーバーレイ / プレイヤー / プレイヤー情報";
            if (g_page == OverlayPage::PlayerDetail)
                return "プレイヤー情報" + std::to_string(g_selectedPlayer + 1);

            const MenuPage *page = FindMenuPage(g_page);
            return page == nullptr ? "Overlay" : page->title;
        }

        std::string GetItemLabel(u32 index)
        {
            if (g_page == OverlayPage::Keyboard)
                return MH4G::GameKeyboard::IsBusy() ? "Keyboard open..." : "Open keyboard";
            if (g_page == OverlayPage::PlayerInfo)
                return "プレイヤー情報" + std::to_string(index + 1);

            const MenuPage *page = FindMenuPage(g_page);
            if (page == nullptr || index >= page->itemCount)
                return "";

            return page->items[index].label;
        }

        bool OpenOverlayKeyboard()
        {
            MH4G::GameKeyboard::ClearResult();
            g_keyboardStatus = "Opening...";
            for (u32 i = 0; i < 4; ++i)
                g_keyboardRaw[i] = 0;

            if (!MH4G::GameKeyboard::Open(g_keyboardResult))
            {
                g_keyboardStatus = "Open failed";
                return false;
            }

            g_keyboardStatus = "Input in progress";
            return true;
        }

        void CaptureKeyboardResult()
        {
            if (!MH4G::GameKeyboard::HasResult())
                return;

            const MH4G::GameKeyboard::Result result = MH4G::GameKeyboard::GetResult();
            if (result == MH4G::GameKeyboard::Result::Accepted)
            {
                const u16 *raw = MH4G::GameKeyboard::GetResultText();
                for (u32 i = 0; i < 4; ++i)
                    g_keyboardRaw[i] = raw[i];
                g_keyboardResult = MH4G::GameKeyboard::GetResultUtf8();
                g_keyboardStatus = "Accepted";
            }
            else if (result == MH4G::GameKeyboard::Result::Canceled)
            {
                g_keyboardStatus = "Canceled";
                for (u32 i = 0; i < 4; ++i)
                    g_keyboardRaw[i] = 0;
            }
            else if (result == MH4G::GameKeyboard::Result::Failed)
            {
                g_keyboardStatus = "Failed";
                for (u32 i = 0; i < 4; ++i)
                    g_keyboardRaw[i] = 0;
            }

            MH4G::GameKeyboard::ClearResult();
        }

        void EnterSelection()
        {
            if (g_page == OverlayPage::PlayerInfo)
            {
                g_selectedPlayer = g_cursor;
                g_page = OverlayPage::PlayerDetail;
                ResetCursor();
                return;
            }

            if (g_page == OverlayPage::Keyboard)
            {
                OpenOverlayKeyboard();
                return;
            }

            const MenuPage *page = FindMenuPage(g_page);
            if (page != nullptr && g_cursor < page->itemCount)
            {
                g_page = page->items[g_cursor].target;
                if (g_page == OverlayPage::Keyboard)
                    OpenOverlayKeyboard();
            }
            ResetCursor();
        }

        void GoBack()
        {
            switch (g_page)
            {
            case OverlayPage::Root:
                g_open = false;
                break;
            case OverlayPage::PlayerInfo:
                g_page = OverlayPage::Player;
                ResetCursor();
                break;
            case OverlayPage::PlayerDetail:
                g_page = OverlayPage::PlayerInfo;
                g_cursor = g_selectedPlayer;
                EnsureCursorVisible();
                break;
            case OverlayPage::Keyboard:
                if (MH4G::GameKeyboard::IsBusy())
                    MH4G::GameKeyboard::Close();
                g_page = OverlayPage::Root;
                ResetCursor();
                break;
            default:
            {
                const MenuPage *page = FindMenuPage(g_page);
                g_page = page == nullptr ? OverlayPage::Root : page->back;
                ResetCursor();
                break;
            }
            }
        }

        void UpdateInput()
        {
            MH4G::GameKeyboard::Update();
            CaptureKeyboardResult();

            if (MH4G::GameKeyboard::IsBusy())
                return;

            if (Controller::IsKeyPressed(Start))
            {
                g_open = !g_open;
                if (g_open)
                {
                    g_page = OverlayPage::Root;
                    ResetCursor();
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

                EnsureCursorVisible();

                if (Controller::IsKeyPressed(A))
                    EnterSelection();
            }
        }

        void DrawHeader(const Screen &screen, const Color &accent)
        {
            screen.DrawRect(Layout::PanelX, Layout::PanelY, Layout::PanelW, Layout::HeaderH, accent);
            screen.DrawSysfont(GetTitle(), Layout::PanelX + 8, Layout::PanelY + 5, Color::Black);
            screen.DrawRect(Layout::PanelX, Layout::PanelY, Layout::PanelW, Layout::PanelH, Color::White, false);
        }

        void DrawHelp(const Screen &screen)
        {
            screen.Draw("Aで開く、Bで戻る", Layout::PanelX + 10, Layout::HelpY,
                        Color(190, 198, 210, 255), Color(0, 0, 0, 0));
        }

        void DrawDirectory(const Screen &screen)
        {
            const Color accent = Color(245, 202, 92, 255);
            const Color panel = Color(20, 24, 30, 185);
            const Color selected = Color(245, 202, 92, 255);
            const Color row = Color(38, 44, 54, 185);

            EnsureCursorVisible();

            screen.DrawRect(Layout::PanelX, Layout::PanelY, Layout::PanelW, Layout::PanelH, panel);
            DrawHeader(screen, accent);

            const u32 itemCount = GetItemCount();
            const u32 lastVisible = itemCount < g_scroll + Layout::VisibleRows ? itemCount : g_scroll + Layout::VisibleRows;
            for (u32 i = g_scroll; i < lastVisible; ++i)
            {
                const u32 y = Layout::ListY + (i - g_scroll) * Layout::RowStep;
                const bool active = i == g_cursor;
                screen.DrawRect(Layout::ListX, y, Layout::RowW, Layout::RowH, active ? selected : row);
                screen.DrawRect(Layout::ListX, y, Layout::RowW, Layout::RowH,
                                active ? Color::White : Color(84, 96, 110, 210), false);

                const std::string prefix = g_page == OverlayPage::PlayerInfo ? "  " : "> ";
                screen.DrawSysfont(prefix + GetItemLabel(i), Layout::ListX + 8, y + 3,
                                   active ? Color::Black : Color::White);
            }

            if (g_scroll > 0)
                screen.Draw("^", Layout::PanelX + Layout::PanelW - 16, Layout::ListY - 12,
                            Color::White, Color(0, 0, 0, 0));
            if (lastVisible < itemCount)
                screen.Draw("v", Layout::PanelX + Layout::PanelW - 16,
                            Layout::ListY + Layout::VisibleRows * Layout::RowStep - 4,
                            Color::White, Color(0, 0, 0, 0));

            const u32 typeInfo = g_draw == nullptr ? 0 : MH4G::GUI::CDraw::GetTypeInfo();
            screen.Draw("Game draw: " + (g_draw == nullptr ? std::string("none") : Hex(reinterpret_cast<u32>(g_draw))),
                        Layout::PanelX + 10, Layout::PanelY + 184,
                        Color(190, 198, 210, 255), Color(0, 0, 0, 0));
            screen.Draw("Type: " + Hex(typeInfo), Layout::PanelX + 10, Layout::PanelY + 198,
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
            const Color panel = Color(20, 24, 30, 185);
            const u32 statX = Layout::PanelX + 14;

            screen.DrawRect(Layout::PanelX, Layout::PanelY, Layout::PanelW, Layout::PanelH, panel);
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
                screen.DrawSysfont("Player data is not available.", statX, 72, Color::White);
                DrawHelp(screen);
                return;
            }

            char buffer[64];
            std::snprintf(buffer, sizeof(buffer), "%.2f, %.2f, %.2f",
                          work->positionX, work->positionY, work->positionZ);

            DrawStat(screen, statX, 62, "Player", Hex(reinterpret_cast<u32>(player)));
            DrawStat(screen, statX, 78, "Base work", Hex(reinterpret_cast<u32>(work)));
            DrawStat(screen, statX, 94, "Position", buffer);

            std::snprintf(buffer, sizeof(buffer), "%u / %u",
                          static_cast<unsigned int>(work->health),
                          static_cast<unsigned int>(work->maximumHealth));
            DrawStat(screen, statX, 110, "Health", buffer);

            std::snprintf(buffer, sizeof(buffer), "%.2f / %.2f",
                          work->stamina, work->maximumStamina);
            DrawStat(screen, statX, 126, "Stamina", buffer);

            std::snprintf(buffer, sizeof(buffer), "%.2f", work->playerSpeed);
            DrawStat(screen, statX, 142, "Speed", buffer);

            std::snprintf(buffer, sizeof(buffer), "%ld", static_cast<long>(work->roomNumber));
            DrawStat(screen, statX, 158, "Room", buffer);

            std::snprintf(buffer, sizeof(buffer), "%u / %u / %u",
                          static_cast<unsigned int>(work->playerState),
                          static_cast<unsigned int>(work->actionState),
                          static_cast<unsigned int>(work->actionNumber));
            DrawStat(screen, statX, 174, "State", buffer);

            std::snprintf(buffer, sizeof(buffer), "%u / %u",
                          static_cast<unsigned int>(work->moveEnabled),
                          static_cast<unsigned int>(work->drawEnabled));
            DrawStat(screen, statX, 190, "Move/Draw", buffer);

            DrawHelp(screen);
        }

        void DrawKeyboardPage(const Screen &screen)
        {
            const Color accent = Color(116, 196, 255, 255);
            const Color panel = Color(20, 24, 30, 185);
            const Color selected = Color(116, 196, 255, 255);
            const u32 statX = Layout::PanelX + 14;

            screen.DrawRect(Layout::PanelX, Layout::PanelY, Layout::PanelW, Layout::PanelH, panel);
            DrawHeader(screen, accent);

            screen.DrawRect(Layout::ListX, Layout::ListY, Layout::RowW, Layout::RowH, selected);
            screen.DrawRect(Layout::ListX, Layout::ListY, Layout::RowW, Layout::RowH, Color::White, false);
            screen.DrawSysfont("> " + GetItemLabel(0), Layout::ListX + 8, Layout::ListY + 3, Color::Black);

            DrawStat(screen, statX, 88, "Status", g_keyboardStatus);
            DrawStat(screen, statX, 108, "Result",
                     g_keyboardResult.empty() ? std::string("(empty)") : g_keyboardResult);

            char buffer[64];
            std::snprintf(buffer, sizeof(buffer), "%04X %04X %04X %04X",
                          static_cast<unsigned int>(g_keyboardRaw[0]),
                          static_cast<unsigned int>(g_keyboardRaw[1]),
                          static_cast<unsigned int>(g_keyboardRaw[2]),
                          static_cast<unsigned int>(g_keyboardRaw[3]));
            DrawStat(screen, statX, 128, "UTF-16", buffer);

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
            else if (g_page == OverlayPage::Keyboard)
                DrawKeyboardPage(screen);
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
        MH4G::GameKeyboard::Close();
    }
}
