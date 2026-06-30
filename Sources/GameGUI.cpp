#include "MH4G/GameGUI.hpp"
#include "MH4G/GUI.hpp"

#include <3ds.h>
#include <CTRPluginFramework.hpp>

#include <cstdio>
#include <string>

namespace CTRPluginFramework
{
    namespace
    {
        MenuEntry *g_entry = nullptr;
        MH4G::GUI::CDraw *g_cdraw = nullptr;
        MH4G::GUI::RLayoutFont *g_font = nullptr;
        MH4G::GUI::SwkbdGUIManager g_manager;
        MH4G::GUI::SwkbdGUIWidget g_headerWidget;
        MH4G::GUI::SwkbdGUIButton g_buttons[3];
        bool g_osdRunning = false;
        bool g_altTheme = false;
        bool g_nativePartsReady = false;
        u32 g_frameCounter = 0;
        u32 g_selectedButton = 0;

        struct NativeButtonView
        {
            const char *label;
            u32 iconId;
            u32 x;
            u32 width;
        };

        struct TouchButtonView
        {
            u32 x;
            u32 y;
            u32 width;
            u32 height;
        };

        const NativeButtonView g_buttonViews[] = {
            {"Theme", 0, 42, 92},
            {"Close", 1, 142, 92},
            {"Icon", 2, 242, 92},
        };

        const TouchButtonView g_touchButtonViews[] = {
            {18, 74, 84, 34},
            {118, 74, 84, 34},
            {218, 74, 84, 34},
        };

        std::string Hex(u32 value)
        {
            char buffer[16];
            std::snprintf(buffer, sizeof(buffer), "0x%08lX", static_cast<unsigned long>(value));
            return std::string(buffer);
        }

        void EnsureCDraw()
        {
            if (g_cdraw != nullptr)
                return;

            g_cdraw = MH4G::GUI::CDraw::Create();
        }

        void EnsureFont()
        {
            if (g_font != nullptr)
                return;

            g_font = MH4G::GUI::RLayoutFont::Create();
        }

        void EnsureNativeParts()
        {
            if (g_nativePartsReady)
                return;

            g_manager.Initialize();
            g_headerWidget.Initialize();

            for (u32 i = 0; i < sizeof(g_buttons) / sizeof(g_buttons[0]); ++i)
                g_buttons[i].Initialize();

            g_nativePartsReady = true;
        }

        bool AreNativePartsValid()
        {
            if (!g_nativePartsReady || !g_manager.HasGameVTable() || !g_headerWidget.HasGameVTable())
                return false;

            for (u32 i = 0; i < sizeof(g_buttons) / sizeof(g_buttons[0]); ++i)
            {
                if (!g_buttons[i].HasGameVTable())
                    return false;
            }

            return true;
        }

        void RunButtonAction(u32 index)
        {
            if (index == 0)
                g_altTheme = !g_altTheme;
            else if (index == 1 && g_entry != nullptr)
                g_entry->Disable();
        }

        bool ContainsTouch(const TouchButtonView &view, const touchPosition &touch)
        {
            return touch.px >= view.x && touch.px < view.x + view.width &&
                   touch.py >= view.y && touch.py < view.y + view.height;
        }

        bool HandleTouchInput()
        {
            if ((hidKeysDown() & KEY_TOUCH) == 0)
                return false;

            touchPosition touch;
            hidTouchRead(&touch);

            for (u32 i = 0; i < sizeof(g_touchButtonViews) / sizeof(g_touchButtonViews[0]); ++i)
            {
                if (!ContainsTouch(g_touchButtonViews[i], touch))
                    continue;

                g_selectedButton = i;
                RunButtonAction(i);
                return true;
            }

            return false;
        }

        void DrawButton(const Screen &screen, u32 index, const Color &accent)
        {
            const NativeButtonView &view = g_buttonViews[index];
            volatile MH4G::GUI::SwkbdGUIButton &button = g_buttons[index];
            const volatile MH4G::GUI::GameIconInfo *iconInfo =
                MH4G::GUI::UIdSprite::GetIconInfo(static_cast<u8>(view.iconId));
            const bool selected = index == g_selectedButton;
            const Color fill = selected ? accent : Color(32, 36, 42, 220);
            const Color border = selected ? Color::White : Color(96, 108, 120, 255);
            const Color text = selected ? Color::Black : Color::White;
            std::string label = std::string(view.label) + " #" +
                                std::to_string(static_cast<u32>(iconInfo->tileIndex));
            if (index == 2 && (g_frameCounter & 0x20) != 0)
                label += ".";

            screen.DrawRect(view.x, 214, view.width, 20, fill);
            screen.DrawRect(view.x, 214, view.width, 20, border, false);
            screen.Draw(label, view.x + 7, 220, text, Color(0, 0, 0, 0));

            if (!button.HasGameVTable())
                screen.Draw("!", view.x + view.width - 12, 220, Color::Red, Color(0, 0, 0, 0));
        }

        void DrawTouchButton(const Screen &screen, u32 index, const Color &accent)
        {
            const NativeButtonView &button = g_buttonViews[index];
            const TouchButtonView &touch = g_touchButtonViews[index];
            const bool selected = index == g_selectedButton;
            const Color fill = selected ? accent : Color(32, 36, 42, 230);
            const Color border = selected ? Color::White : Color(96, 108, 120, 255);
            const Color text = selected ? Color::Black : Color::White;

            screen.DrawRect(touch.x, touch.y, touch.width, touch.height, fill);
            screen.DrawRect(touch.x, touch.y, touch.width, touch.height, border, false);
            screen.Draw(button.label, touch.x + 11, touch.y + 12, text, Color(0, 0, 0, 0));
        }

        bool DrawTouchControls(const Screen &screen)
        {
            if (screen.IsTop || g_entry == nullptr || !g_entry->IsActivated())
                return false;

            const Color panel = g_altTheme ? Color(20, 50, 45, 225) : Color(26, 30, 38, 225);
            const Color accent = g_altTheme ? Color(72, 214, 160, 255) : Color(255, 196, 72, 255);
            const Color muted = Color(170, 178, 190, 255);

            screen.DrawRect(8, 44, 304, 108, panel);
            screen.DrawRect(8, 44, 304, 108, accent, false);
            screen.DrawRect(8, 44, 304, 24, accent);
            screen.Draw("Touch controls", 18, 52, Color::Black, Color(0, 0, 0, 0));
            screen.Draw("Tap a native GUI button", 18, 122, muted, Color(0, 0, 0, 0));

            for (u32 i = 0; i < sizeof(g_touchButtonViews) / sizeof(g_touchButtonViews[0]); ++i)
                DrawTouchButton(screen, i, accent);

            return true;
        }

        bool DrawGameGui(const Screen &screen)
        {
            if (g_entry == nullptr || !g_entry->IsActivated())
                return false;

            if (!screen.IsTop)
                return DrawTouchControls(screen);

            HandleTouchInput();

            if (!g_entry->IsActivated())
                return false;

            if (Controller::IsKeyPressed(B))
            {
                g_entry->Disable();
                return false;
            }

            if (Controller::IsKeyPressed(A))
                RunButtonAction(g_selectedButton);

            if (Controller::IsKeyPressed(Left))
                g_selectedButton = (g_selectedButton + 2) % 3;
            else if (Controller::IsKeyPressed(Right))
                g_selectedButton = (g_selectedButton + 1) % 3;

            if (Controller::IsKeyPressed(X))
                g_altTheme = !g_altTheme;

            EnsureCDraw();
            EnsureFont();
            EnsureNativeParts();
            ++g_frameCounter;

            const Color panel = g_altTheme ? Color(20, 50, 45, 225) : Color(26, 30, 38, 225);
            const Color accent = g_altTheme ? Color(72, 214, 160, 255) : Color(255, 196, 72, 255);
            const Color muted = Color(170, 178, 190, 255);

            screen.DrawRect(26, 12, 348, 224, panel);
            screen.DrawRect(26, 12, 348, 224, accent, false);
            screen.DrawRect(26, 12, 348, 24, accent);

            screen.Draw("MH4G Game GUI", 36, 20, Color::Black, Color(0, 0, 0, 0));
            screen.Draw("Swkbd GUI parts model", 36, 44, Color::White, Color(0, 0, 0, 0));

            const u32 drawAddress = reinterpret_cast<u32>(g_cdraw);
            const u32 vtable = g_cdraw == nullptr ? 0 : g_cdraw->GetVTable();
            const u32 typeInfo = MH4G::GUI::CDraw::GetTypeInfo();

            screen.Draw("CDraw object", 42, 68, muted, Color(0, 0, 0, 0));
            screen.Draw(g_cdraw == nullptr ? "create failed" : Hex(drawAddress),
                        146, 68, Color::White, Color(0, 0, 0, 0));
            screen.Draw("Type info", 42, 84, muted, Color(0, 0, 0, 0));
            screen.Draw(Hex(typeInfo), 146, 84, Color::White, Color(0, 0, 0, 0));
            screen.Draw("VTable", 42, 100, muted, Color(0, 0, 0, 0));
            screen.Draw(vtable == 0 ? "unavailable" : Hex(vtable),
                        146, 100, Color::White, Color(0, 0, 0, 0));

            const u32 fontAddress = reinterpret_cast<u32>(g_font);
            const u32 fontVTable = g_font == nullptr ? 0 : g_font->GetVTable();
            const u32 fontResource = g_font == nullptr ? 0 : g_font->GetResource();
            const u32 fontTypeInfo = MH4G::GUI::RLayoutFont::GetTypeInfo();

            screen.Draw("Game font", 42, 120, muted, Color(0, 0, 0, 0));
            screen.Draw(g_font == nullptr ? "create failed" : Hex(fontAddress),
                        146, 120, Color::White, Color(0, 0, 0, 0));
            screen.Draw("Font type", 42, 136, muted, Color(0, 0, 0, 0));
            screen.Draw(Hex(fontTypeInfo), 146, 136, Color::White, Color(0, 0, 0, 0));
            screen.Draw("Font vtbl", 42, 152, muted, Color(0, 0, 0, 0));
            screen.Draw(fontVTable == 0 ? "unavailable" : Hex(fontVTable),
                        146, 152, Color::White, Color(0, 0, 0, 0));
            screen.Draw("Resource", 42, 168, muted, Color(0, 0, 0, 0));
            screen.Draw(fontResource == 0 ? "not loaded" : Hex(fontResource),
                        146, 168, Color::White, Color(0, 0, 0, 0));

            screen.Draw("Swkbd mgr", 42, 184, muted, Color(0, 0, 0, 0));
            screen.Draw(Hex(g_manager.vtable), 146, 184, Color::White, Color(0, 0, 0, 0));
            screen.Draw("Parts", 42, 200, muted, Color(0, 0, 0, 0));
            screen.Draw(AreNativePartsValid() ? "native vtables ready" : "invalid vtable",
                        146, 200, AreNativePartsValid() ? Color::White : Color::Red,
                        Color(0, 0, 0, 0));

            for (u32 i = 0; i < sizeof(g_buttons) / sizeof(g_buttons[0]); ++i)
                DrawButton(screen, i, accent);

            return true;
        }

        void StartOSD()
        {
            if (g_osdRunning)
                return;

            OSD::Run(DrawGameGui);
            g_osdRunning = true;
        }

        void StopOSD()
        {
            if (!g_osdRunning)
                return;

            OSD::Stop(DrawGameGui);
            g_osdRunning = false;
        }
    }

    void UpdateGameGuiDemo(MenuEntry *entry)
    {
        g_entry = entry;

        if (entry != nullptr && entry->IsActivated())
            StartOSD();
        else
            StopOSD();
    }

    void StopGameGuiDemo()
    {
        StopOSD();
        g_entry = nullptr;
        g_cdraw = nullptr;
        g_font = nullptr;
        g_nativePartsReady = false;
        g_selectedButton = 0;
    }
}
