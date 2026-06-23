#ifndef MH4G_LOGGER_HPP
#define MH4G_LOGGER_HPP

#include <string>
#include "types.h"

namespace CTRPluginFramework
{
    class MenuEntry;

    void UpdateChatAutoDump(MenuEntry *entry);
    bool IsChatAutoDumpEnabled();

    bool DumpChatMessage(const std::string &playerName, const std::string &chatContent);
    bool DumpChatMessage(u32 playerSlot, const std::string &chatContent);
    bool DumpChatMessageUtf16(u32 playerSlot, u32 chatAddress, u32 byteLength);
}

#endif
