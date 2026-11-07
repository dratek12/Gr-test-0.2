#include "LegitBot.h"
#include "Global.h"

static Vec3 Hitbox(const CPlayer& p, int idx) {
    static const float h[5] = { 72.f, 65.f, 55.f, 45.f, 35.f };
    if (idx < 0 || idx > 4) idx = 0;
    return Vec3{ p.origin.x, p.origin.y, p.origin.z + h[idx] };
}

static float FovOf(const Vec3& from, const Vec3& to, const Vec3& view) {
    Vec3 d = to - from;
    float len = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
    if (len < 0.001f) return 0.f;
    float yaw = std::atan2(-d.x, d.y) * 57.29578f;
    float pitch = std::atan2(d.z, std::sqrt(d.x * d.x + d.y * d.y)) * 57.29578f;
    float dy = std::remainder(yaw - view.y, 360.f);
    float dp = std::remainder(pitch - view.x, 360.f);
    return std::sqrt(dy * dy + dp * dp);
}

static void Click() {
    INPUT in[2]{};
    in[0].type = INPUT_MOUSE; in[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    in[1].type = INPUT_MOUSE; in[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(2, in, sizeof(INPUT));
}

void RunAim() {
    auto& c = G::Ui.state.cs2;
    if (!G::LocalAlive || G::Players.empty() || !c.aimKey || !c.aim) return;
    if (!Sdk::Foreground()) return;
    if (!(GetAsyncKeyState(c.aimKey) & 0x8000)) return;

    uintptr_t client = Sdk::Client();
    Vec3 eye{ G::LocalOrigin.x, G::LocalOrigin.y, G::LocalOrigin.z + 64.f };
    Vec3 view = Sdk::R<Vec3>(client + Off::ViewAngles);

    CPlayer* target = nullptr;
    Vec3 dest{};
    float best = c.fov;

    for (auto& p : G::Players) {
        if (!p.valid || p.team == G::LocalTeam) continue;
        for (int hb = 0; hb < 5; ++hb) {
            Vec3 pos = Hitbox(p, hb);
            float f = FovOf(eye, pos, view);
            if (f < best) { best = f; target = &p; dest = pos; }
        }
    }
    if (!target) return;
    if (Sdk::RandRange(0.f, 100.f) > c.hitchance) return;

    Vec3 d = dest - eye;
    float yaw = std::atan2(-d.x, d.y) * 57.29578f;
    float pitch = std::atan2(d.z, std::sqrt(d.x * d.x + d.y * d.y)) * 57.29578f;
    float dy = std::remainder(yaw - view.y, 360.f);
    float dp = std::remainder(pitch - view.x, 360.f);

    if (c.rcs) {
        dy -= G::Punch.y * 2.f * Sdk::RandRange(c.recoilMin, c.recoilMax) / 100.f;
        dp -= G::Punch.x * 2.f * Sdk::RandRange(c.recoilMin, c.recoilMax) / 100.f;
    }
    if (c.randomize > 0.f) {
        dy += Sdk::RandRange(-c.randomize, c.randomize);
        dp += Sdk::RandRange(-c.randomize, c.randomize);
    }

    float scale = (G::Sens > 0.0001f ? G::Sens : 1.f) * 0.022f;
    float smooth = std::max(c.smooth, 1.f);
    Sdk::SendMouse(dy / scale / smooth, -dp / scale / smooth);

    if (c.autoFire) Click();
}

void RunTrigger() {
    auto& c = G::Ui.state.cs2;
    if (!c.trigger || !c.triggerKey || !G::LocalAlive || !Sdk::Foreground()) return;
    if (!(GetAsyncKeyState(c.triggerKey) & 0x8000)) return;

    ImVec2 size = ImGui::GetIO().DisplaySize;
    float cx = size.x * 0.5f, cy = size.y * 0.5f;
    float m[16]{};
    memcpy(m, reinterpret_cast<void*>(Sdk::Client() + Off::ViewMatrix), sizeof(m));

    for (auto& p : G::Players) {
        if (!p.valid || p.team == G::LocalTeam) continue;
        ImVec2 s;
        if (!Sdk::W2S(m, p.head, s, size)) continue;
        float dx = s.x - cx, dy = s.y - cy;
        if (std::sqrt(dx * dx + dy * dy) > 25.f) continue;
        if (Sdk::RandRange(0.f, 100.f) > c.hitchance) continue;
        Click();
        return;
    }
}