#include "MH4G/GameKeyboard.hpp"

#include <CTRPluginFramework.hpp>

namespace
{
    namespace Addresses
    {
        constexpr u32 SharedKeyboardManagerPtr = 0x00F886A0;
        constexpr u32 ConfigureKeyboard = 0x002A5C3C;
        constexpr u32 OpenKeyboard = 0x002A5E94;
        constexpr u32 PollKeyboard = 0x002A5D9C;
        constexpr u32 CloseKeyboard = 0x002A62B4;
        constexpr u32 GetKeyboardText = 0x002A0688;
    }

    enum class KeyboardState
    {
        Idle,
        Waiting
    };

    constexpr u32 BufferChars = MH4G::GameKeyboard::DefaultMaxChars;

    CTRPluginFramework::MenuEntry *g_entry = nullptr;
    KeyboardState g_state = KeyboardState::Idle;
    MH4G::GameKeyboard::Result g_result = MH4G::GameKeyboard::Result::None;
    u32 g_maxChars = BufferChars;
    u16 g_initialText[BufferChars + 1] = {};
    u16 g_resultText[BufferChars + 1] = {};

    u32 ClampMaxChars(u32 maxChars)
    {
        if (maxChars == 0 || maxChars > BufferChars)
            return BufferChars;

        return maxChars;
    }

    u32 GetKeyboardManager()
    {
        volatile u32 *slot = reinterpret_cast<volatile u32 *>(Addresses::SharedKeyboardManagerPtr);
        return *slot;
    }

    bool Configure(u32 manager)
    {
        typedef int (*ConfigureFn)(u32 manager, int mode, int maxLength, int enableExtraChecks);
        ConfigureFn configure = reinterpret_cast<ConfigureFn>(Addresses::ConfigureKeyboard);

        // mode 1 is the game's normal text keyboard path; maxLength 0 uses 0x1C internally.
        return configure(manager, 1, g_maxChars, 0) != 0;
    }

    bool OpenKeyboard(u32 manager)
    {
        typedef bool (*OpenFn)(u32 manager, u32 flags, const u16 *initialText);
        OpenFn open = reinterpret_cast<OpenFn>(Addresses::OpenKeyboard);
        return open(manager, 0, g_initialText);
    }

    int Poll(u32 manager)
    {
        typedef int (*PollFn)(u32 manager);
        PollFn poll = reinterpret_cast<PollFn>(Addresses::PollKeyboard);
        return poll(manager);
    }

    void CloseKeyboard(u32 manager)
    {
        typedef void (*CloseFn)(u32 manager);
        CloseFn close = reinterpret_cast<CloseFn>(Addresses::CloseKeyboard);
        close(manager);
    }

    const u16 *GetText(u32 manager)
    {
        typedef const u16 *(*GetTextFn)(u32 manager);
        GetTextFn getText = reinterpret_cast<GetTextFn>(Addresses::GetKeyboardText);
        return getText(manager);
    }

    void CopyText(u16 *destination, const u16 *source, u32 maxChars)
    {
        if (source == nullptr)
        {
            destination[0] = 0;
            return;
        }

        for (u32 i = 0; i < maxChars; ++i)
        {
            destination[i] = source[i];
            if (source[i] == 0)
            {
                return;
            }
        }

        destination[maxChars] = 0;
    }

    void AppendUtf8(std::string &output, u32 codepoint)
    {
        if (codepoint <= 0x7F)
        {
            output += static_cast<char>(codepoint);
        }
        else if (codepoint <= 0x7FF)
        {
            output += static_cast<char>(0xC0 | (codepoint >> 6));
            output += static_cast<char>(0x80 | (codepoint & 0x3F));
        }
        else if (codepoint <= 0xFFFF)
        {
            output += static_cast<char>(0xE0 | (codepoint >> 12));
            output += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            output += static_cast<char>(0x80 | (codepoint & 0x3F));
        }
        else
        {
            output += static_cast<char>(0xF0 | (codepoint >> 18));
            output += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
            output += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            output += static_cast<char>(0x80 | (codepoint & 0x3F));
        }
    }

    std::string Utf16ToUtf8(const u16 *text, u32 maxChars)
    {
        std::string output;

        for (u32 i = 0; i < maxChars && text[i] != 0; ++i)
        {
            u32 codepoint = text[i];

            if (0xD800 <= codepoint && codepoint <= 0xDBFF && i + 1 < maxChars)
            {
                const u32 low = text[i + 1];
                if (0xDC00 <= low && low <= 0xDFFF)
                {
                    codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (low - 0xDC00);
                    ++i;
                }
            }

            AppendUtf8(output, codepoint);
        }

        return output;
    }

    u32 ReadUtf8Codepoint(const std::string &input, u32 &offset)
    {
        const u8 first = static_cast<u8>(input[offset++]);

        if ((first & 0x80) == 0)
            return first;

        u32 codepoint = 0xFFFD;
        u32 extraBytes = 0;

        if ((first & 0xE0) == 0xC0)
        {
            codepoint = first & 0x1F;
            extraBytes = 1;
        }
        else if ((first & 0xF0) == 0xE0)
        {
            codepoint = first & 0x0F;
            extraBytes = 2;
        }
        else if ((first & 0xF8) == 0xF0)
        {
            codepoint = first & 0x07;
            extraBytes = 3;
        }

        for (u32 i = 0; i < extraBytes && offset < input.size(); ++i)
        {
            const u8 next = static_cast<u8>(input[offset]);
            if ((next & 0xC0) != 0x80)
                break;

            codepoint = (codepoint << 6) | (next & 0x3F);
            ++offset;
        }

        return codepoint;
    }

    void Utf8ToUtf16(u16 *destination, const std::string &input, u32 maxChars)
    {
        u32 output = 0;
        u32 offset = 0;

        while (offset < input.size() && output < maxChars)
        {
            const u32 codepoint = ReadUtf8Codepoint(input, offset);
            if (codepoint <= 0xFFFF)
            {
                destination[output++] = static_cast<u16>(codepoint);
            }
            else if (output + 1 < maxChars)
            {
                const u32 shifted = codepoint - 0x10000;
                destination[output++] = static_cast<u16>(0xD800 | (shifted >> 10));
                destination[output++] = static_cast<u16>(0xDC00 | (shifted & 0x3FF));
            }
        }

        destination[output] = 0;
    }
}

namespace MH4G
{
    namespace GameKeyboard
    {
        bool Open(const u16 *initialText, u32 maxChars)
        {
            if (g_state == KeyboardState::Waiting)
                return false;

            g_maxChars = ClampMaxChars(maxChars);
            g_result = Result::None;
            CopyText(g_initialText, initialText, g_maxChars);

            const u32 manager = GetKeyboardManager();
            if (manager == 0)
            {
                g_result = Result::Failed;
                return false;
            }

            if (!Configure(manager) || !OpenKeyboard(manager))
            {
                g_result = Result::Failed;
                return false;
            }

            g_state = KeyboardState::Waiting;
            return true;
        }

        bool Open(const std::string &initialText, u32 maxChars)
        {
            u16 text[BufferChars + 1] = {};
            const u32 clampedMax = ClampMaxChars(maxChars);

            Utf8ToUtf16(text, initialText, clampedMax);
            return Open(text, clampedMax);
        }

        void Update()
        {
            if (g_state != KeyboardState::Waiting)
                return;

            const u32 manager = GetKeyboardManager();
            if (manager == 0)
            {
                g_state = KeyboardState::Idle;
                g_result = Result::Failed;
                return;
            }

            const int result = Poll(manager);
            if (result == 0)
                return;

            if (result == 1)
            {
                CopyText(g_resultText, GetText(manager), g_maxChars);
                CopyText(g_initialText, g_resultText, g_maxChars);
                g_result = Result::Accepted;
            }
            else
            {
                g_result = Result::Canceled;
            }

            CloseKeyboard(manager);
            g_state = KeyboardState::Idle;
        }

        void Close()
        {
            const u32 manager = GetKeyboardManager();
            if (manager != 0 && g_state == KeyboardState::Waiting)
                CloseKeyboard(manager);

            if (g_state == KeyboardState::Waiting)
                g_result = Result::Canceled;

            g_state = KeyboardState::Idle;
        }

        void ClearResult()
        {
            g_result = Result::None;
            g_resultText[0] = 0;
        }

        bool IsBusy()
        {
            return g_state == KeyboardState::Waiting;
        }

        bool HasResult()
        {
            return g_result != Result::None;
        }

        Result GetResult()
        {
            return g_result;
        }

        const u16 *GetResultText()
        {
            return g_resultText;
        }

        std::string GetResultUtf8()
        {
            return Utf16ToUtf8(g_resultText, BufferChars);
        }

        std::string GetResultAscii()
        {
            return GetResultUtf8();
        }
    }
}

namespace CTRPluginFramework
{
    void UpdateGameKeyboard(MenuEntry *entry)
    {
        g_entry = entry;
        if (entry == nullptr || !entry->IsActivated())
        {
            MH4G::GameKeyboard::Close();
            return;
        }

        if (!MH4G::GameKeyboard::IsBusy() && MH4G::GameKeyboard::HasResult())
            MH4G::GameKeyboard::ClearResult();

        if (!MH4G::GameKeyboard::IsBusy() && !MH4G::GameKeyboard::HasResult())
        {
            if (!MH4G::GameKeyboard::Open())
            {
                MessageBox("Game Keyboard", "Failed to open the game's software keyboard.")();
                entry->Disable();
                return;
            }
        }

        MH4G::GameKeyboard::Update();
        if (MH4G::GameKeyboard::HasResult())
            entry->Disable();
    }

    void StopGameKeyboard()
    {
        MH4G::GameKeyboard::Close();
        g_entry = nullptr;
    }
}
