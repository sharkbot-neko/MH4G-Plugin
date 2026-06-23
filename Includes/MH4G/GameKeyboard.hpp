#ifndef MH4G_GAMEKEYBOARD_HPP
#define MH4G_GAMEKEYBOARD_HPP

#include <string>
#include "types.h"

namespace MH4G
{
    namespace GameKeyboard
    {
        constexpr u32 DefaultMaxChars = 0x1C;

        enum class Result
        {
            None,
            Accepted,
            Canceled,
            Failed
        };

        bool Open(const u16 *initialText = nullptr, u32 maxChars = DefaultMaxChars);
        bool Open(const std::string &initialText, u32 maxChars = DefaultMaxChars);
        void Update();
        void Close();
        void ClearResult();

        bool IsBusy();
        bool HasResult();
        Result GetResult();
        const u16 *GetResultText();
        std::string GetResultUtf8();
        std::string GetResultAscii();
    }
}

namespace CTRPluginFramework
{
    class MenuEntry;

    void UpdateGameKeyboard(MenuEntry *entry);
    void StopGameKeyboard();
}

#endif
