#include "ESP.h"
#include "Global.h"

void RefreshPlayers() {
    G::Players.clear();
    G::LocalAlive = false;
    uintptr_t client = Sdk::Client();
    if (!client) return;

    uintptr_t local = Sdk::R<uintptr_t>(client + Off::LocalPlayer);
    if (!local) return;

    int lhp = Sdk::R<int>(local + Off::Health);
    uintptr_t scene = Sdk::R<uintptr_t>(local + Off::GameSceneNode);
    if (lhp > 0 && lhp <= 100 && scene) {
        G::LocalAlive = true;
        G::LocalOrigin = Sdk::R<Vec3>(scene + 0xC8);
        G::LocalTeam = Sdk::R<int>(local + Off::Team);
        float s = Sdk::R<float>(client + Off::Sensitivity + Off::SensitivityVal);
        if (s > 0.0001f) G::Sens = s;
        uintptr_t punch = Sdk::R<uintptr_t>(local + Off::AimPunch);
        G::Punch = punch ? Sdk::R<Vec3>(punch + Off::PunchAngle) : Vec3{};
    }

    uintptr_t list = Sdk::R<uintptr_t>(client + Off::EntityList);
    if (!list) return;

    for (int i = 1; i < 65; ++i) {
        uintptr_t chunk = Sdk::R<uintptr_t>(list + 0x10 + (i / 512) * 8);
        if (!chunk) continue;
        uintptr_t ident = chunk + (i % 512) * 0x70;
        uintptr_t ent = Sdk::R<uintptr_t>(ident);
        if (!ent) continue;
        if (static_cast<int>(Sdk::R<uint32_t>(ident + 0x10) & 0x7FFF) != i) continue;

        int hp = Sdk::R<int>(ent + Off::Health);
        if (hp <= 0 || hp > 100) continue;
        if (Sdk::R<uint8_t>(ent + Off::LifeState)) continue;
        if (Sdk::R<uint8_t>(ent + Off::Dormant)) continue;

        uintptr_t es = Sdk::R<uintptr_t>(ent + Off::GameSceneNode);
        if (!es) continue;
        Vec3 org = Sdk::R<Vec3>(es + 0xC8);
        if (org.x == 0.f && org.y == 0.f && org.z == 0.f) continue;

        CPlayer p;
        p.origin = org;
        p.head = Vec3{ org.x, org.y, org.z + 72.f };
        p.health = hp;
        p.team = Sdk::R<int>(ent + Off::Team);
        p.valid = true;

        uintptr_t ws = Sdk::R<uintptr_t>(ent + Off::WeaponServices);
        if (ws) {
            uint32_t wh = Sdk::R<uint32_t>(ws + Off::ActiveWeapon) & 0x7FFF;
            if (wh) {
                uintptr_t wc = Sdk::R<uintptr_t>(list + 0x10 + (wh / 512) * 8);
                if (wc) {
                    uintptr_t w = Sdk::R<uintptr_t>(wc + (wh % 512) * 0x70);
                    if (w) p.ammo = Sdk::R<int>(w + Off::Clip1);
                }
            }
        }
        G::Players.push_back(p);
    }
}

void DrawESP() {
    if (G::Players.empty()) return;
    ImDrawList* d = ImGui::GetForegroundDrawList();
    ImVec2 size = ImGui::GetIO().DisplaySize;

    float m[16]{};
    memcpy(m, reinterpret_cast<void*>(Sdk::Client() + Off::ViewMatrix), sizeof(m));

    auto& st = G::Ui.state;
    ImU32 col = ImGui::ColorConvertFloat4ToU32(st.preview.color);

    for (auto& p : G::Players) {
        if (!p.valid) continue;
        if (st.cs2.team && p.team == G::LocalTeam) continue;

        ImVec2 top, bot;
        if (!Sdk::W2S(m, p.head, top, size)) continue;
        if (!Sdk::W2S(m, p.origin, bot, size)) continue;
        float h = bot.y - top.y;
        if (h < 4 || h > 900) continue;
        float w = h * 0.45f;

        if (st.preview.boxes) {
            d->AddRect(ImVec2(top.x - w / 2 - 1, top.y - 1), ImVec2(top.x + w / 2 + 1, bot.y + 1), IM_COL32(0, 0, 0, 255));
            d->AddRect(ImVec2(top.x - w / 2, top.y), ImVec2(top.x + w / 2, bot.y), col);
        }
        if (st.preview.health) {
            float frac = std::clamp(p.health / 100.f, 0.f, 1.f);
            d->AddRectFilled(ImVec2(top.x - w / 2 - 6, top.y), ImVec2(top.x - w / 2 - 2, bot.y), IM_COL32(0, 0, 0, 180));
            d->AddRectFilled(ImVec2(top.x - w / 2 - 5, bot.y - (bot.y - top.y) * frac),
                             ImVec2(top.x - w / 2 - 3, bot.y), IM_COL32(60, 220, 60, 255));
        }
        if (st.preview.names) d->AddText(ImVec2(top.x - w / 2, top.y - 14), col, "player");
        if (st.cs2.ammo) d->AddText(ImVec2(top.x - w / 2, bot.y + 2), col, std::to_string(p.ammo).c_str());
        if (st.preview.snaplines) d->AddLine(ImVec2(size.x * 0.5f, size.y), ImVec2(top.x, bot.y), col);
        if (st.preview.head) d->AddCircle(ImVec2(top.x, top.y + h * 0.08f), h * 0.10f, col, 16);
    }

    if (st.cs2.fovCircle && G::LocalAlive) {
        float px = st.cs2.fov * (size.y / 90.f);
        d->AddCircle(ImVec2(size.x * 0.5f, size.y * 0.5f), px, IM_COL32(255, 255, 255, 60), 64, 1.f);
    }
    if (st.cs2.watermark)
        d->AddText(ImVec2(12, 10), IM_COL32(255, 255, 255, 210), "cs2esp");
}