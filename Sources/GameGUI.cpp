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
            ++g_frameCounter;

            const Color panel = g_altTheme ? Color(20, 50, 45, 225) : Color(26, 30, 38, 225);
            const Color accent = g_altTheme ? Color(72, 214, 160, 255) : Color(255, 196, 72, 255);
            const Color muted = Color(170, 178, 190, 255);

            screen.DrawRect(26, 24, 348, 156, panel);
            screen.DrawRect(26, 24, 348, 156, accent, false);
            screen.DrawRect(26, 24, 348, 24, accent);

            screen.Draw("MH4G Game GUI", 36, 32, Color::Black, Color(0, 0, 0, 0));
            screen.Draw("CDraw-backed overlay demo", 36, 58, Color::White, Color(0, 0, 0, 0));

            const u32 drawAddress = reinterpret_cast<u32>(g_cdraw);
            const u32 vtable = g_cdraw == nullptr ? 0 : g_cdraw->GetVTable();
            const u32 typeInfo = MH4G::GUI::CDraw::GetTypeInfo();

            screen.Draw("CDraw object", 42, 82, muted, Color(0, 0, 0, 0));
            screen.Draw(g_cdraw == nullptr ? "create failed" : Hex(drawAddress),
                        146, 82, Color::White, Color(0, 0, 0, 0));
            screen.Draw("Type info", 42, 98, muted, Color(0, 0, 0, 0));
            screen.Draw(Hex(typeInfo), 146, 98, Color::White, Color(0, 0, 0, 0));
            screen.Draw("VTable", 42, 114, muted, Color(0, 0, 0, 0));
            screen.Draw(vtable == 0 ? "unavailable" : Hex(vtable),
                        146, 114, Color::White, Color(0, 0, 0, 0));

            DrawButton(screen, 42, 144, 92, "A Theme", g_altTheme, accent);
            DrawButton(screen, 142, 144, 92, "B Close", false, accent);
            DrawButton(screen, 242, 144, 92, g_frameCounter & 0x20 ? "Live" : "Live.", true, accent);

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
    }
}
