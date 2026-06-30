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

            constexpr u32 RLayoutFontCreate = 0x00CAF734;
            constexpr u32 RLayoutFontGetTypeInfo = 0x00CAF794;
            constexpr u32 RLayoutFontTypeInfo = 0x010B8FB8;
            constexpr u32 RLayoutFontRegisteredVTable = 0x00E42B8C;
            constexpr u32 RLayoutFontInstanceVTable = 0x00E2F274;
            constexpr u32 RegisterRLayoutFontType = 0x00D15AFC;

            constexpr u32 UIdSpriteTypeInfo = 0x010B79B0;
            constexpr u32 UIdSpriteBaseTypeInfo = 0x010B699C;
            constexpr u32 UIdSpriteVTable = 0x00E50CE0;
            constexpr u32 UIdSpriteDestroy = 0x00C09BE8;
            constexpr u32 RegisterUIdSpriteType = 0x00D08EAC;

            constexpr u32 GameIconInfoTable = 0x00FEF9DC;
            constexpr u32 GameSpriteBehaviorTable = 0x00FEEEC0;
            constexpr u32 GameSpriteScaleTable = 0x00FF3418;
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

        class RLayoutFont
        {
        public:
            static RLayoutFont *Create()
            {
                typedef RLayoutFont *(*Creator)();
                Creator creator = reinterpret_cast<Creator>(Addresses::RLayoutFontCreate);
                return creator();
            }

            static u32 GetTypeInfo()
            {
                typedef u32 (*Getter)();
                Getter getter = reinterpret_cast<Getter>(Addresses::RLayoutFontGetTypeInfo);
                return getter();
            }

            u32 GetVTable() const volatile
            {
                return _vtable;
            }

            u32 GetResource() const volatile
            {
                return _resource;
            }

        private:
            volatile u32 _vtable;
            u8 _unknown0004[0x48];
            volatile u32 _drawPriority;       // 0x004C; constructor initializes this to 0x10.
            u8 _unknown0050[0x14];
            volatile u32 _resource;           // 0x0064; backing font resource pointer, if loaded.
            u8 _opaque[Sizes::RLayoutFont - 0x68];
        };

        struct SwkbdGUIWidget
        {
            volatile u32 vtable;
            u8 opaque[Sizes::SwkbdGUIWidget - sizeof(u32)];

            void Initialize()
            {
                vtable = Addresses::SwkbdGUIWidgetVTable;
                for (size_t i = 0; i < sizeof(opaque); ++i)
                    opaque[i] = 0;
            }

            bool HasGameVTable() const volatile
            {
                return vtable == Addresses::SwkbdGUIWidgetVTable;
            }
        };

        struct SwkbdGUIManager
        {
            volatile u32 vtable;
            u8 opaque[Sizes::SwkbdGUIManager - sizeof(u32)];

            void Initialize()
            {
                vtable = Addresses::SwkbdGUIManagerVTable;
                for (size_t i = 0; i < sizeof(opaque); ++i)
                    opaque[i] = 0;
            }

            bool HasGameVTable() const volatile
            {
                return vtable == Addresses::SwkbdGUIManagerVTable;
            }
        };

        struct SwkbdGUIButton
        {
            volatile u32 vtable;
            u8 opaque[Sizes::SwkbdGUIButton - sizeof(u32)];

            void Initialize()
            {
                vtable = Addresses::SwkbdGUIButtonVTable;
                for (size_t i = 0; i < sizeof(opaque); ++i)
                    opaque[i] = 0;
            }

            bool HasGameVTable() const volatile
            {
                return vtable == Addresses::SwkbdGUIButtonVTable;
            }
        };

        struct GameIconInfo
        {
            u8 texturePage;
            u8 tileIndex;
            u8 variant;
            u8 flags;
            u32 extra;
        };

        class UIdSprite
        {
        public:
            u32 GetVTable() const volatile
            {
                return _vtable;
            }

            u32 GetResource() const volatile
            {
                return _resource;
            }

            static const volatile GameIconInfo *GetIconInfo(u8 iconId)
            {
                const volatile GameIconInfo *table =
                    reinterpret_cast<const volatile GameIconInfo *>(Addresses::GameIconInfoTable);
                return &table[iconId];
            }

        private:
            volatile u32 _vtable;
            u8 _unknown0004[0x2C];
            volatile u32 _resource;           // 0x0030; sprite-owned backing resource, if loaded.
            u8 _opaque[Sizes::UIdSprite - 0x34];
        };

        static_assert(sizeof(CDraw) == Sizes::CDraw, "CDraw size mismatch");
        static_assert(sizeof(RLayoutFont) == Sizes::RLayoutFont, "RLayoutFont size mismatch");
        static_assert(sizeof(GameIconInfo) == 0x08, "icon info size mismatch");
        static_assert(sizeof(UIdSprite) == Sizes::UIdSprite, "UIdSprite size mismatch");
        static_assert(sizeof(SwkbdGUIWidget) == Sizes::SwkbdGUIWidget, "GUI widget size mismatch");
        static_assert(sizeof(SwkbdGUIManager) == Sizes::SwkbdGUIManager, "GUI manager size mismatch");
        static_assert(sizeof(SwkbdGUIButton) == Sizes::SwkbdGUIButton, "GUI button size mismatch");
    }
}

#endif
