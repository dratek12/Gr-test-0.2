#pragma once
#include "CommonIncludes.h"

struct Vec3 { float x, y, z; };
inline Vec3 operator-(const Vec3& a, const Vec3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }

namespace Off {
    enum : uintptr_t {
        EntityList = 0x271EBA8, LocalPlayer = 0x2569948, ViewMatrix = 0x256F250,
        ViewAngles = 0x2580098, Sensitivity = 0x2566C48,
        Health = 0x34C, LifeState = 0x354, Team = 0x3E7,
        GameSceneNode = 0x330, Dormant = 0x103,
        WeaponServices = 0x12F0, ActiveWeapon = 0x60, Clip1 = 0x1928,
        AimPunch = 0x1598, PunchAngle = 0x50, SensitivityVal = 0x58
    };
}

struct CPlayer {
    Vec3 origin{}, head{};
    int health = 0, team = 0, ammo = 0;
    bool valid = false;
};

namespace Sdk {
    template <typename T> T R(uintptr_t a);
    uintptr_t Client();
    bool W2S(const float m[16], const Vec3& w, ImVec2& out, ImVec2 size);
    void SendMouse(float dx, float dy);
    float RandRange(float a, float b);
    bool Foreground();
}