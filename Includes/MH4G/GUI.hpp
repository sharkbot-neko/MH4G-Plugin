#ifndef MH4G_GUI_HPP
#define MH4G_GUI_HPP

#include <cstddef>
#include "types.h"

namespace MH4G
{
    namespace GUI
    {
        namespace Addresses
        {
            // Address layout from the analyzed Japanese 000400000011D700 CXI.
            constexpr u32 CDrawCreate = 0x00CCF244;
            constexpr u32 CDrawGetTypeInfo = 0x00CCF2EC;
            constexpr u32 RegisterCDrawType = 0x00D2E7B8;

            constexpr u32 CDrawTypeInfo = 0x010B693C;
            constexpr u32 CDrawBaseTypeInfo = 0x01091B0C;
            constexpr u32 CDrawVTable = 0x00E36424;

            constexpr u32 SwkbdGUIWidgetVTable = 0x00E4DC9C;
            constexpr u32 SwkbdGUIManagerVTable = 0x00E4DCD8;
            constexpr u32 SwkbdGUIButtonVTable = 0x00E4DBA4;

            constexpr u32 RegisterSwkbdGUIWidgetType = 0x00D23E30;
            constexpr u32 RegisterSwkbdGUIManagerType = 0x00D26538;
            constexpr u32 RegisterSwkbdGUIButtonType = 0x00D23CD0;
        }

        namespace Sizes
        {
            constexpr size_t CDraw = 0x6AF0;
            constexpr size_t SwkbdGUIWidget = 0x14;
            constexpr size_t SwkbdGUIManager = 0x30;
            constexpr size_t SwkbdGUIButton = 0x8C;
            constexpr size_t UIdSprite = 0x70;
            constexpr size_t RLayoutFont = 0xA0;
        }

        class CDraw
        {
        public:
            static CDraw *Create()
            {
                typedef CDraw *(*Creator)();
                Creator creator = reinterpret_cast<Creator>(Addresses::CDrawCreate);
                return creator();
            }

            static u32 GetTypeInfo()
            {
                typedef u32 (*Getter)();
                Getter getter = reinterpret_cast<Getter>(Addresses::CDrawGetTypeInfo);
                return getter();
            }

            u32 GetVTable() const volatile
            {
                return _vtable;
            }

        private:
            volatile u32 _vtable;
            u8 _opaque[Sizes::CDraw - sizeof(u32)];
        };

        struct SwkbdGUIWidget
        {
            volatile u32 vtable;
            u8 opaque[Sizes::SwkbdGUIWidget - sizeof(u32)];
        };

        struct SwkbdGUIManager
        {
            volatile u32 vtable;
            u8 opaque[Sizes::SwkbdGUIManager - sizeof(u32)];
        };

        struct SwkbdGUIButton
        {
            volatile u32 vtable;
            u8 opaque[Sizes::SwkbdGUIButton - sizeof(u32)];
        };

        static_assert(sizeof(CDraw) == Sizes::CDraw, "CDraw size mismatch");
        static_assert(sizeof(SwkbdGUIWidget) == Sizes::SwkbdGUIWidget, "GUI widget size mismatch");
        static_assert(sizeof(SwkbdGUIManager) == Sizes::SwkbdGUIManager, "GUI manager size mismatch");
        static_assert(sizeof(SwkbdGUIButton) == Sizes::SwkbdGUIButton, "GUI button size mismatch");
    }
}

#endif
