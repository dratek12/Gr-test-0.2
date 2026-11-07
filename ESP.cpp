#include "ESP.h"
#include "Global.h"

void RefreshPlayers() {
    G::Players.clear();
    G::LocalAlive = false;
    uintptr_t client = Sdk::Client();
    if (!client) return;

    uintptr_t local = Sdk::R<uintptr_t>(client + Off::LocalPlayer);
    if (!local) return;

    G::LocalTeam = Sdk::R<uint8_t>(local + Off::Team);
    G::LocalAlive = Sdk::R<uint8_t>(local + Off::LifeState) == 0;
    G::Punch = Sdk::R<Vec3>(Sdk::R<uintptr_t>(local + Off::AimPunch) + Off::PunchAngle);
    G::Sens = Sdk::R<float>(Sdk::R<uintptr_t>(client + Off::Sensitivity) + Off::SensitivityVal);

    uintptr_t es = Sdk::R<uintptr_t>(local + Off::GameSceneNode);
    if (es) G::LocalOrigin = Sdk::R<Vec3>(es + Off::AbsOrigin);

    uintptr_t list = Sdk::R<uintptr_t>(client + Off::EntityList);
    if (!list) return;

    for (int i = 1; i < 65; ++i) {
        uintptr_t chunk = Sdk::R<uintptr_t>(list + 0x10 + (i / 512) * 8);
        if (!chunk) continue;
        uintptr_t ent = Sdk::R<uintptr_t>(chunk + (i % 512) * 0x70);
        if (!ent) continue;

        int hp = Sdk::R<int>(ent + Off::Health);
        if (hp <= 0 || hp > 100) continue;
        if (Sdk::R<uint8_t>(ent + Off::LifeState)) continue;

        uintptr_t scene = Sdk::R<uintptr_t>(ent + Off::GameSceneNode);
        if (!scene) continue;
        if (Sdk::R<uint8_t>(scene + Off::Dormant)) continue;

        Vec3 org = Sdk::R<Vec3>(scene + Off::AbsOrigin);
        if (org.x == 0.f && org.y == 0.f) continue;

        CPlayer p{};
        p.origin = org;
        p.head = Vec3{ org.x, org.y, org.z + 72.f };
        p.health = hp;
        p.team = Sdk::R<uint8_t>(ent + Off::Team);
        p.valid = true;

        uintptr_t ws = Sdk::R<uintptr_t>(ent + Off::WeaponServices);
        if (ws) {
            uintptr_t wp = Sdk::R<uintptr_t>(ws + Off::ActiveWeapon);
            if (wp) p.ammo = Sdk::R<int>(wp + Off::Clip1);
        }

        G::Players.push_back(p);
    }
}

void DrawESP() {
    if (G::Players.empty() || !G::Ready) return;

    ImDrawList* d = ImGui::GetForegroundDrawList();
    ImVec2 size = ImGui::GetIO().DisplaySize;
    float m[16]{};
    memcpy(m, reinterpret_cast<void*>(Sdk::Client() + Off::ViewMatrix), sizeof(m));

    for (auto& p : G::Players) {
        if (G::Ui.state.cs2.team && p.team == G::LocalTeam) continue;

        ImVec2 feet, head;
        if (!Sdk::W2S(m, p.origin, feet, size)) continue;
        if (!Sdk::W2S(m, p.head, head, size)) continue;

        float h = feet.y - head.y;
        if (h < 4.f || h > 900.f) continue;
        float w = h * 0.45f;

        ImU32 col = p.team == 2 ? IM_COL32(235, 170, 70, 255) : IM_COL32(90, 175, 255, 255);

        if (G::Ui.state.preview.boxes)
            d->AddRect(ImVec2(head.x - w / 2, head.y), ImVec2(head.x + w / 2, feet.y), col, 0.f, 1.5f);

        if (G::Ui.state.preview.health) {
            float hh = (feet.y - head.y) * (p.health / 100.f);
            d->AddRectFilled(ImVec2(head.x - w / 2 - 6, head.y), ImVec2(head.x - w / 2 - 2, feet.y), IM_COL32(10, 10, 10, 200));
            d->AddRectFilled(ImVec2(head.x - w / 2 - 6, feet.y - hh), ImVec2(head.x - w / 2 - 2, feet.y), IM_COL32(80, 220, 80, 255));
        }

        if (G::Ui.state.preview.names)
            d->AddText(ImVec2(head.x - 20, head.y - 14), IM_COL32(230, 230, 230, 255), "player");

        if (G::Ui.state.preview.head)
            d->AddRect(ImVec2(head.x - 5, head.y - 5), ImVec2(head.x + 5, head.y + 5), col, 0.f, 1.f);

        if (G::Ui.state.preview.snaplines)
            d->AddLine(ImVec2(size.x / 2, size.y), ImVec2(feet.x, feet.y), col, 1.f);
    }
}