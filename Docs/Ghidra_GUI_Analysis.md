# MH4G Native GUI Ghidra Analysis

Target: Japanese MH4G title `000400000011D700`.

This note records the Ghidra findings needed to call game-side GUI code from the
CTRPF plugin. It focuses on entry points that are already safe enough to wrap,
plus the remaining unknowns before arbitrary native GUI widgets can be built.

## Confirmed Entry Points

| Purpose | Ghidra name | Address | Plugin wrapper |
| --- | --- | --- | --- |
| Create cDraw | `MH4G_CDraw_Create` | `0x00CCF244` | `MH4G::GUI::CDraw::Create()` |
| Create rLayoutFont | `MH4G_RLayoutFont_Create` | `0x00CAF734` | `MH4G::GUI::RLayoutFont::Create()` |
| Configure software keyboard | `MH4G_Swkbd_Configure` | `0x002A5C3C` | inside `MH4G::GameKeyboard::Open()` |
| Open software keyboard | `MH4G_Swkbd_Open` | `0x002A5E94` | inside `MH4G::GameKeyboard::Open()` |
| Poll software keyboard | `MH4G_Swkbd_Poll` | `0x002A5D9C` | `MH4G::GameKeyboard::Update()` |
| Close software keyboard | `MH4G_Swkbd_Close` | `0x002A62B4` | `MH4G::GameKeyboard::Close()` |
| Get keyboard UTF-16 buffer | `MH4G_Swkbd_GetText` | `0x002A0688` | `MH4G::GameKeyboard::GetResultText()` |

## Names Applied In Ghidra

Functions:

- `0x00CCF244` -> `MH4G_CDraw_Create`
- `0x00CAF734` -> `MH4G_RLayoutFont_Create`
- `0x002A5C3C` -> `MH4G_Swkbd_Configure`
- `0x002A5E94` -> `MH4G_Swkbd_Open`
- `0x002A5D9C` -> `MH4G_Swkbd_Poll`
- `0x002A62B4` -> `MH4G_Swkbd_Close`
- `0x002A0688` -> `MH4G_Swkbd_GetText`
- `0x00D2E7B8` -> `MH4G_Register_CDraw_Type`
- `0x00D15AFC` -> `MH4G_Register_RLayoutFont_Type`
- `0x00D08EAC` -> `MH4G_Register_UIdSprite_Type`
- `0x00D23E30` -> `MH4G_Register_Swkbd_Widget_Type`
- `0x00D26538` -> `MH4G_Register_Swkbd_Manager_Type`
- `0x00D23CD0` -> `MH4G_Register_Swkbd_Button_Type`

Globals:

- `0x010B693C` -> `g_dwMH4G_CDraw_TypeInfo`
- `0x00E36424` -> `g_dwMH4G_CDraw_VTable`
- `0x010B8FB8` -> `g_dwMH4G_RLayoutFont_TypeInfo`
- `0x00E2F274` -> `g_dwMH4G_RLayoutFont_InstanceVTable`
- `0x010B79B0` -> `g_dwMH4G_UIdSprite_TypeInfo`
- `0x00FEF9DC` -> `g_abMH4G_GameIconInfoTable`
- `0x00F886A0` -> `g_dwMH4G_SharedKeyboardManagerPtr`

## cDraw

`MH4G_CDraw_Create()` allocates `0x6AF0` bytes through the game allocator,
writes the cDraw vtable, and initializes internal state.

Observed behavior:

- Allocation size is `0x6AF0`.
- Allocation alignment is `0x10`.
- The first DWORD receives the cDraw vtable value.
- An internal object array starts at `object + 0x1210` (`puVar3 + 0x484`) and
  is initialized with 5 elements.
- Holding the created pointer in plugin code is reasonable, but the safe draw
  API surface is not fully identified yet.

Current plugin code only uses this for native GUI asset/path probing in
`Sources/OverlayMenu.cpp` and `Sources/GameGUI.cpp`.

## rLayoutFont

`MH4G_RLayoutFont_Create()` allocates `0xA0` bytes, runs the base constructor
path, writes the rLayoutFont vtable, and initializes known fields.

Confirmed offsets:

- `+0x00`: vtable
- `+0x4C`: draw priority, initialized to `0x10`
- `+0x64`: resource pointer, initialized to `0`

`Includes/MH4G/GUI.hpp` matches these layout findings.

## uIdSprite And Icon Metadata

`MH4G_Register_UIdSprite_Type()` registers `uIdSprite` with size `0x70`.

`0x00FEF9DC` is an icon atlas metadata table. The current wrapper treats each
entry as 8 bytes:

```cpp
struct GameIconInfo
{
    u8 texturePage;
    u8 tileIndex;
    u8 variant;
    u8 flags;
    u32 extra;
};
```

Unknowns:

- Meaning of `extra`.
- Entry layout for `GameSpriteBehaviorTable`.
- Entry layout for `GameSpriteScaleTable`.
- `uIdSprite` constructor/create function and position/draw functions.

## nSwkbd GUI Types

Type registration functions confirm these names and object sizes:

| Type name | Registration function | Size |
| --- | --- | --- |
| `nSwkbd::GUI::Widget` | `0x00D23E30` | `0x14` |
| `nSwkbd::GUI::Manager` | `0x00D26538` | `0x30` |
| `nSwkbd::GUI::Button` | `0x00D23CD0` | `0x8C` |

Vtable candidates:

- `nSwkbd::GUI::Widget`: `0x00E4DC9C`
- `nSwkbd::GUI::Manager`: `0x00E4DCD8`
- `nSwkbd::GUI::Button`: `0x00E4DBA4`

These belong to the game's software keyboard UI. To build custom native GUI,
the next useful xref path is the existing Manager/Button construction flow.
That should reveal widget insertion, label text, coordinates, and press/update
callbacks.

## Software Keyboard

The shared manager pointer slot is at `0x00F886A0`.

`MH4G_Swkbd_Configure(void *manager, int mode, int maxLength, int enableExtraChecks)`:

- `manager + 0x2F18`: internal keyboard object pointer
- `manager + 0x2F20`: max text length
- `manager + 0x2F24`: UTF-16 result buffer
- `mode == 1 && maxLength == 0`: max length becomes `0x1C`
- `mode == 2`: max length becomes 4
- `mode == 3`: max length becomes `0x0E`

`MH4G_Swkbd_Poll(void *manager)`:

- `0`: still waiting
- `1`: accepted non-empty/non-space text; text is in `manager + 0x2F24`
- `-1`: canceled or closed

The current `MH4G::GameKeyboard` wrapper is consistent with these findings.

## Next Analysis Targets

1. Find the constructor/initializer for `nSwkbd::GUI::Manager`.
2. Find the constructor/initializer for `nSwkbd::GUI::Button`.
3. Identify label text, rectangle/position, state, and callback fields in Button.
4. Identify the function that attaches widgets to the cDraw/update tree.
5. Identify `uIdSprite` creation, icon-id assignment, and draw-position setters.
6. Verify on hardware whether these calls must only run from the game frame
   thread or synchronized frame callback.

The currently safe native GUI entry point is the game's software keyboard.
`cDraw`, `rLayoutFont`, and the icon metadata path are confirmed for creation
or read-only probing, but arbitrary widget insertion into the game GUI tree is
not confirmed yet.
