#ifndef MH4G_PLAYER_HPP
#define MH4G_PLAYER_HPP

#include <cstddef>
#include "types.h"

namespace MH4G
{
    namespace Addresses
    {
        // Address layout from the analyzed Japanese 000400000011D700 CXI.
        constexpr u32 SPlayerSingleton = 0x00F89EF0;
        constexpr u32 GameManagerSingleton = 0x01028B9C;
        constexpr u32 SPlayerGetPlayerByIndex = 0x00AD5A78;
        constexpr u32 SPlayerFindActivePlayer = 0x00AD3A7C;
    }

    struct PlayerWork
    {
        u8 _unknown0000[0x18];
        float rate;                         // 0x0018
        u8 _unknown001C[0x0C];
        float gravityRate;                  // 0x0028 (mRateG)
    };

    struct BasePlayerWork
    {
        u8 _unknown0000[0x04];
        s32 roomNumber;                     // 0x0004 (r_no)
        u8 _unknown0008[0x08];
        float positionX;                    // 0x0010
        float positionY;                    // 0x0014
        float positionZ;                    // 0x0018
        u8 _unknown001C[0x04];
        s32 vectorX;                        // 0x0020
        s32 vectorY;                        // 0x0024
        s32 vectorZ;                        // 0x0028
        u32 playerDataAddress;              // 0x002C; name is UTF-16 at +0x18
        u8 unitKind;                        // 0x0030
        u8 playerType;                      // 0x0031 (pl_type)
        u8 _unknown0032[0x02];
        u8 characterType;                   // 0x0034
        u8 _unknown0035[0x317];
        u16 health;                         // 0x034C (hp_vital)
        u16 maximumHealth;                  // 0x034E (max_hp_vital)
        u8 _unknown0350[0x04];
        float stamina;                      // 0x0354 (dash_vital)
        float maximumStamina;               // 0x0358
        u16 recoverableHealth;              // 0x035C (heal_vital)
        u8 _unknown035E[0x02];
        float recoverableHealthTimer;       // 0x0360
        float staminaTimer;                 // 0x0364
        float turboRecoveryTimer;           // 0x0368
        u16 specialGauge;                   // 0x036C
        u16 specialGauge2;                  // 0x036E
        u8 playerState;                     // 0x0370 (pl_st)
        u8 actionState;                     // 0x0371 (act_st)
        u16 actionNumber;                   // 0x0372 (act_no)
        u8 _unknown0374[0x28];
        float playerSpeed;                  // 0x039C
        u8 _unknown03A0[0x34];
        s32 inputHistory[4];                // 0x03D4 (mPlSwitch.bpad)
        u8 _unknown03E4[0x8FF0];
        u8 moveEnabled;                     // 0x93D4 (mMoveFlag)
        u8 drawEnabled;                     // 0x93D5 (mDrawFlag)
    };

    class UPlayer
    {
    public:
        volatile BasePlayerWork *GetBaseWork() volatile
        {
            return reinterpret_cast<volatile BasePlayerWork *>(_baseWorkAddress);
        }

        volatile PlayerWork *GetPlayerWork() volatile
        {
            return reinterpret_cast<volatile PlayerWork *>(_playerWorkAddress);
        }

    private:
        u8 _unknown0000[0x0E30];
        u32 _baseWorkAddress;                // 0x0E30 (pbw)
        u8 _unknown0E34[0x9468];
        u32 _playerWorkAddress;              // 0xA29C (pw)
        u8 _unknownA2A0[0x0A40];
    };

    class SPlayer
    {
    public:
        static volatile SPlayer *GetInstance()
        {
            const volatile u32 *singleton =
                reinterpret_cast<const volatile u32 *>(Addresses::SPlayerSingleton);
            return reinterpret_cast<volatile SPlayer *>(*singleton);
        }

        volatile UPlayer *GetPlayerSlot(u32 index) volatile
        {
            if (index >= 4)
                return nullptr;

            return reinterpret_cast<volatile UPlayer *>(_playerAddresses[index]);
        }

        volatile UPlayer *GetPlayer(u32 index) volatile
        {
            // Use the game function so alternate multiplayer modes select
            // their separate five-entry player array correctly.
            typedef UPlayer *(*Getter)(SPlayer *, u32);
            Getter getter = reinterpret_cast<Getter>(Addresses::SPlayerGetPlayerByIndex);
            return getter(const_cast<SPlayer *>(this), index);
        }

        volatile UPlayer *GetLocalPlayer() volatile
        {
            return GetPlayer(0);
        }

        u32 GetPlayerCount() const volatile
        {
            return _playerCount;
        }

    private:
        u8 _unknown0000[0x0A98];
        u32 _playerAddresses[4];             // 0x0A98..0x0AA4
        u32 _playerCount;                    // 0x0AA8
        u8 _unknown0AAC[0x24];
    };

    static_assert(offsetof(BasePlayerWork, positionX) == 0x10, "position offset mismatch");
    static_assert(offsetof(BasePlayerWork, health) == 0x34C, "health offset mismatch");
    static_assert(offsetof(BasePlayerWork, stamina) == 0x354, "stamina offset mismatch");
    static_assert(offsetof(BasePlayerWork, inputHistory) == 0x3D4, "input offset mismatch");
    static_assert(sizeof(UPlayer) == 0xACE0, "UPlayer size mismatch");
    static_assert(sizeof(SPlayer) == 0xAD0, "SPlayer size mismatch");
}

#endif
