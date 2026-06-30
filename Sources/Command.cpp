#include "MH4G/GameGUI.hpp"
#include "MH4G/GUI.hpp"
#include "MH4G/GameKeyboard.hpp"

#include <3ds.h>
#include <CTRPluginFramework.hpp>

#include <cstdio>
#include <string>

namespace CTRPluginFramework
{
    void ProcessCommand(std::string commandText) {
        MessageBox("Input", commandText)();
        return;
    }

    void GameCommand(void)
    {
        if (Controller::IsKeysDown(R + KEY_UP)) {
            MH4G::GameKeyboard::ClearResult();

            std::string g_keyboardResult;

            if (!MH4G::GameKeyboard::Open(g_keyboardResult))
            {
                return;
            }

            if (!MH4G::GameKeyboard::HasResult())
                return;

            u16 g_keyboardRaw[4] = {};

            const MH4G::GameKeyboard::Result result = MH4G::GameKeyboard::GetResult();
            if (result == MH4G::GameKeyboard::Result::Accepted)
            {
                const u16 *raw = MH4G::GameKeyboard::GetResultText();
                for (u32 i = 0; i < 4; ++i)
                    g_keyboardRaw[i] = raw[i];
                g_keyboardResult = MH4G::GameKeyboard::GetResultUtf8();

                ProcessCommand(g_keyboardResult);
            }
            else if (result == MH4G::GameKeyboard::Result::Canceled)
            {
                for (u32 i = 0; i < 4; ++i)
                    g_keyboardRaw[i] = 0;
            }
            else if (result == MH4G::GameKeyboard::Result::Failed)
            {
                for (u32 i = 0; i < 4; ++i)
                    g_keyboardRaw[i] = 0;
            }

            MH4G::GameKeyboard::ClearResult();
        }
    }
}
