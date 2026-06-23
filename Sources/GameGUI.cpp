#include "MH4G/GameGUI.hpp"
#include "MH4G/GUI.hpp"

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
        bool g_osdRunning = false;
        bool g_altTheme = false;
        u32 g_frameCounter = 0;

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

        void DrawButton(const Screen &screen, u32 x, u32 y, u32 w, const std::string &label,
                        bool selected, const Color &accent)
        {
            const Color fill = selected ? accent : Color(32, 36, 42, 220);
            const Color border = selected ? Color::White : Color(96, 108, 120, 255);
            const Color text = selected ? Color::Black : Color::White;

            screen.DrawRect(x, y, w, 20, fill);
            screen.DrawRect(x, y, w, 20, border, false);
            screen.Draw(label, x + 7, y + 6, text, Color(0, 0, 0, 0));
        }

        bool DrawGameGui(const Screen &screen)
        {
            if (!screen.IsTop || g_entry == nullptr || !g_entry->IsActivated())
                return false;

            if (Controller::IsKeyPressed(B))
            {
                g_entry->Disable();
                return false;
            }

            if (Controller::IsKeyPressed(A))
                g_altTheme = !g_altTheme;

            EnsureCDraw();
            EnsureFont();
            ++g_frameCounter;

            const Color panel = g_altTheme ? Color(20, 50, 45, 225) : Color(26, 30, 38, 225);
            const Color accent = g_altTheme ? Color(72, 214, 160, 255) : Color(255, 196, 72, 255);
            const Color muted = Color(170, 178, 190, 255);

            screen.DrawRect(26, 12, 348, 224, panel);
            screen.DrawRect(26, 12, 348, 224, accent, false);
            screen.DrawRect(26, 12, 348, 24, accent);

            screen.Draw("MH4G Game GUI", 36, 20, Color::Black, Color(0, 0, 0, 0));
            screen.Draw("CDraw-backed overlay demo", 36, 44, Color::White, Color(0, 0, 0, 0));

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
            const volatile MH4G::GUI::GameIconInfo *iconInfo = MH4G::GUI::UIdSprite::GetIconInfo(0);

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

            screen.Draw("Icon table", 42, 184, muted, Color(0, 0, 0, 0));
            screen.Draw(Hex(MH4G::GUI::Addresses::GameIconInfoTable),
                        146, 184, Color::White, Color(0, 0, 0, 0));
            screen.Draw("Icon #0", 42, 200, muted, Color(0, 0, 0, 0));
            screen.Draw("page " + std::to_string(static_cast<u32>(iconInfo->texturePage)) +
                        " tile " + std::to_string(static_cast<u32>(iconInfo->tileIndex)),
                        146, 200, Color::White, Color(0, 0, 0, 0));

            DrawButton(screen, 42, 214, 92, "A Theme", g_altTheme, accent);
            DrawButton(screen, 142, 214, 92, "B Close", false, accent);
            DrawButton(screen, 242, 214, 92, g_frameCounter & 0x20 ? "Icon" : "Icon.", true, accent);

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
    }
}
