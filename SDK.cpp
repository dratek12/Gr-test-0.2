#include "SDK.h"
#include "Global.h"

namespace Sdk {
    template <typename T> T R(uintptr_t a) {
        if (!a) return T{};
        __try { return *reinterpret_cast<T*>(a); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return T{}; }
    }
    template uintptr_t R<uintptr_t>(uintptr_t);
    template uint32_t R<uint32_t>(uintptr_t);
    template uint8_t R<uint8_t>(uintptr_t);
    template int R<int>(uintptr_t);
    template float R<float>(uintptr_t);
    template Vec3 R<Vec3>(uintptr_t);

    uintptr_t Client() { return reinterpret_cast<uintptr_t>(GetModuleHandleW(L"client.dll")); }

    bool W2S(const float m[16], const Vec3& w, ImVec2& out, ImVec2 size) {
        float cx = m[0] * w.x + m[1] * w.y + m[2] * w.z + m[3];
        float cy = m[4] * w.x + m[5] * w.y + m[6] * w.z + m[7];
        float cw = m[12] * w.x + m[13] * w.y + m[14] * w.z + m[15];
        if (cw < 0.01f) return false;
        out.x = (size.x * 0.5f) + (cx / cw) * (size.x * 0.5f);
        out.y = (size.y * 0.5f) - (cy / cw) * (size.y * 0.5f);
        return out.x >= 0 && out.y >= 0 && out.x <= size.x && out.y <= size.y;
    }

    void SendMouse(float dx, float dy) {
        static float accX = 0.f, accY = 0.f;
        accX += dx;
        accY += dy;
        LONG mx = static_cast<LONG>(accX);
        LONG my = static_cast<LONG>(accY);
        if (!mx && !my) return;
        accX -= mx;
        accY -= my;

        INPUT in{};
        in.type = INPUT_MOUSE;
        in.mi.dx = mx;
        in.mi.dy = my;
        in.mi.dwFlags = MOUSEEVENTF_MOVE;
        SendInput(1, &in, sizeof(in));
    }

    float RandRange(float a, float b) {
        static std::mt19937 eng{ std::random_device{}() };
        std::uniform_real_distribution<float> d(a, b);
        return d(eng);
    }

    bool Foreground() { return !G::Window || GetForegroundWindow() == G::Window; }
}