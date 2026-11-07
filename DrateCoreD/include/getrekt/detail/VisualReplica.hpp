#pragma once
// Interactive visual reference only. Every value here is local UI state.
// No aim, input automation, game memory or process interaction is implemented.
#include "GetRektTheme.h"
#include <cstring>
#include <array>
#include <unordered_map>
#include <string>
#include <vector>

namespace getrekt {
    struct VisualReplica {
        int page = 1, legit = 0, legitLeft = 0, legitRight = 2, visuals = 0, esp = 0, misc = 0, skin = 0;
        int weapon = 0, skinChoice = 3, profile = 5;
        struct Cs2State {
            bool aim = false, rcs = false, ammo = false, bombinfo = false, showinfo = false, team = true, bhop = false, trigger = false;
            bool autoFire = true, fovCircle = false, watermark = true;
            float fov = 6.f, smooth = 3.f, dist = 300.f, hitchance = 70.f, delay = 0.f, randomize = 0.6f, recoilMin = 40.f, recoilMax = 80.f;
            int bone = 0, aimKey = 0, triggerKey = 0;
        } cs2;
        int normalWeapon = 0;
        float normalScrollRequest = -1;
        std::array<float, 6> normalFov{ 8,8,8,8,8,8 };
        bool weaponWindow = false, clubWindow = false, spectators = false;
        Theme colors;
        std::unordered_map<std::string, bool> checks;
        std::unordered_map<std::string, float> numbers;
        std::unordered_map<std::string, int> choices;
        std::unordered_map<std::string, int> keybinds;
        std::unordered_map<std::string, std::string> bindingLabels;
        std::unordered_map<std::string, decltype(keybinds)> savedKeybinds;
        std::array<bool, 256> keyDown{};
        std::string capturing;
        bool allowHotkeys = false;
        int reservedMenu = 45, reservedPanic = 46;
        std::unordered_map<std::string, ImVec4> swatches;
        std::unordered_map<std::string, std::array<char, 128>> strings;
        std::vector<std::string> profiles{ "bugged","dobryconfig","legit_beta","M9","monkexlegit_v1","NewSettings","pLegit","pLegit2","pLegit3","pLegitTest" };
        std::unordered_map<std::string, decltype(checks)> savedChecks;
        std::unordered_map<std::string, decltype(numbers)> savedNumbers;
        std::unordered_map<std::string, decltype(choices)> savedChoices;
        ImTextureID playerTexture = 0, knifeTexture = 0, gloveTexture = 0;
        bool disabled = false;
        struct PreviewState {
            bool boxes = false, names = false, health = false, armour = false, weapon = true, snaplines = false, arrows = false, sound = false, head = false, crosshair = false;
            bool glow = false, chams = false;
            ImVec4 color{ 0,.8f,1,1 };
        } preview;
        bool Light() const { return colors.config.windowBg.x + colors.config.windowBg.y + colors.config.windowBg.z > 1.5f; }
        ImU32 Gray(int dark, int light) const { int v = Light() ? light : dark;return IM_COL32(v, v, v, 255); }
        ImU32 AccentText() const { auto c = colors.config.accent;if (Light()) { c.x *= .52f;c.y *= .52f;c.z *= .52f; }return Color(c); }
        void BoundCheck(const char* label, bool& value) {
            checks[Id(label)] = value;value = Check(label, value);
        }

        VisualReplica() {
            colors.config.accent = ImVec4(1, .48f, .045f, 1);
            colors.config.windowBg = ImVec4(.105f, .10f, .095f, 1);
            colors.config.panelBg = ImVec4(.175f, .175f, .175f, 1);
            colors.config.text = ImVec4(.84f, .84f, .84f, 1);
            colors.SetChangelog("GetRekt visual reference", std::vector<Release>{{"1.2", { { ChangeKind::Updated,"Visual reconstruction from the supplied showcase" },{ ChangeKind::Added,"Local demonstration controls and reference previews" } }}});
        }
        ImU32 Color(ImVec4 c) const { return ImGui::ColorConvertFloat4ToU32(c); }
        ImU32 Accent(float alpha = 1) const { auto c = colors.config.accent;c.w = alpha;return Color(c); }
        std::string Id(const char* label) const { return std::to_string(ImGui::GetID(label)); }
        void Label(const char* label) { ImGui::TextUnformatted(label, strstr(label, "##")); }
        static int DefaultBinding(const std::string& label) {
            if (label == "c" || label == "C")return 'C';if (label == "v" || label == "V")return 'V';
            if (label == "TAB")return 9;
            if (label == "MOUSE4" || label == "MOUSE 4")return 5;
            if (label == "MOUSE5" || label == "MOUSE 5")return 6;
            return 0;
        }
        static std::string BindingName(int key) {
            if (!key)return "key not set";
            if (key == 1)return "MOUSE 1";if (key == 2)return "MOUSE 2";
            if (key == 4)return "MOUSE 3";if (key == 5)return "MOUSE 4";if (key == 6)return "MOUSE 5";
            if (key == 9)return "TAB";if (key >= 'A' && key <= 'Z')return std::string(1, static_cast<char>(key));
            if (key >= '0' && key <= '9')return std::string(1, static_cast<char>(key));
            if (key >= 112 && key <= 135)return "F" + std::to_string(key - 111);
        switch (key) { case 8:return "BACKSPACE";case 13:return "ENTER";case 16:return "SHIFT";case 17:return "CTRL";case 18:return "ALT";case 32:return "SPACE";case 37:return "LEFT";case 38:return "UP";case 39:return "RIGHT";case 40:return "DOWN";default:return "KEY " + std::to_string(key); }
        }
        void CancelBinding() { capturing.clear();keyDown.fill(false); }
        bool HandleBindingKey(int key, bool down) {
            if (key <= 0 || key >= 256)return false;
            bool edge = down && !keyDown[key];keyDown[key] = down;
            if (!capturing.empty()) {
                if (!edge)return true;
                if (key == 27) { keybinds[capturing] = 0;capturing.clear();return true; }
                if (key == reservedMenu || key == reservedPanic || key == 121 || key == 91 || key == 92 || key == 93)return true;
                if (key == 1 || key == 2 || key == 4 || key == 5 || key == 6 || key >= 8) { keybinds[capturing] = key;capturing.clear(); }
                return true;
            }
            if (edge && allowHotkeys)for (const auto& binding : keybinds)if (binding.second == key)checks[binding.first] = !checks[binding.first];
            return false;
        }
        void Section(const char* label) {
            auto p = ImGui::GetCursorScreenPos(); auto* d = ImGui::GetWindowDrawList();
            const float right = ImGui::GetWindowPos().x + ImGui::GetWindowWidth() - 20;
            d->AddLine(ImVec2(p.x + 2, p.y + 6), ImVec2(p.x + 20, p.y + 6), Accent(.55f));
            d->AddText(ImVec2(p.x + 30, p.y), AccentText(), label);
            float end = p.x + 38 + ImGui::CalcTextSize(label).x;
            if (end < right)d->AddLine(ImVec2(end, p.y + 6), ImVec2(right, p.y + 6), Accent(.55f));
            ImGui::Dummy(ImVec2(0, 14));
        }
        bool Check(const char* label, bool initial = false) {
            auto key = Id(label);auto [it, inserted] = checks.try_emplace(key, initial);(void)inserted;
            bool& value = it->second;auto p = ImGui::GetCursorScreenPos();
            std::string text = label;const auto bracket = text.find('[');bool hasBinding = bracket != std::string::npos;
            if (hasBinding) { auto close = text.find(']', bracket);keybinds.try_emplace(key, DefaultBinding(text.substr(bracket + 1, close - bracket - 1)));text.resize(bracket);while (!text.empty() && text.back() == ' ')text.pop_back();bindingLabels[key] = text; }
            ImGui::PushID(label);bool clicked = ImGui::InvisibleButton("check", ImVec2(hasBinding ? 20 + ImGui::CalcTextSize(text.c_str()).x : ImGui::GetContentRegionAvail().x, 14));
            if (clicked && !disabled)value = !value;
            auto* d = ImGui::GetWindowDrawList();
            d->AddRectFilledMultiColor(ImVec2(p.x, p.y + 1), ImVec2(p.x + 12, p.y + 13), Gray(24, 220), Gray(24, 220), Gray(5, 165), Gray(5, 165));
            if (value)d->AddRectFilled(ImVec2(p.x + 2, p.y + 3), ImVec2(p.x + 10, p.y + 11), Accent());
            d->AddText(ImVec2(p.x + 20, p.y + 1), Color(colors.config.text), text.c_str());
            if (hasBinding) {
                ImGui::SameLine(0, 5);auto pos = ImGui::GetCursorScreenPos();
                std::string badge = "[" + (capturing == key ? std::string("press a key...") : BindingName(keybinds[key])) + "]";
                if (ImGui::InvisibleButton("binding", ImVec2(ImGui::CalcTextSize(badge.c_str()).x + 2, 14)) && !disabled) { capturing = key;keyDown.fill(false); }
                d->AddText(ImVec2(pos.x, pos.y + 1), capturing == key ? AccentText() : Color(colors.config.text), badge.c_str());
                if (ImGui::IsItemHovered())ImGui::SetTooltip("Click to bind a key or mouse button. Escape clears. Menu, panic and F10 are reserved.");
            }
            ImGui::PopID();return value;
        }
        void Checks(std::initializer_list<const char*> labels) { for (auto label : labels)Check(label); }
        void Slider(const char* label, float initial, float min, float max, const char* format = "%.0f") {
            auto [it, inserted] = numbers.try_emplace(Id(label), initial);(void)inserted;
            ImGui::PushID(label);auto p = ImGui::GetCursorScreenPos();
            ImGui::InvisibleButton("##value", ImVec2(174, 16));
            if (ImGui::IsItemActive() && !disabled)it->second = min + std::clamp((ImGui::GetIO().MousePos.x - p.x - 5) / 164.f, 0.f, 1.f) * (max - min);
            auto* d = ImGui::GetWindowDrawList();
            d->AddRectFilledMultiColor(p, ImVec2(p.x + 174, p.y + 16), Gray(26, 205), Gray(26, 205), Gray(5, 165), Gray(5, 165));
            float x = p.x + 2 + std::clamp((it->second - min) / (max - min), 0.f, 1.f) * 163;
            d->AddRectFilled(ImVec2(x, p.y + 2), ImVec2(x + 9, p.y + 14), Accent(), 2);
            char number[64]{};snprintf(number, sizeof(number), format, static_cast<double>(it->second));auto size = ImGui::CalcTextSize(number);
            d->AddText(ImVec2(p.x + (174 - size.x) / 2, p.y + 1), Color(colors.config.text), number);
            ImGui::SameLine();Label(label);ImGui::PopID();
        }
        void Combo(const char* label, const char* list, int initial = 0) {
            auto [it, inserted] = choices.try_emplace(Id(label), initial);(void)inserted;
            ImGui::PushID(label);ImGui::PushItemWidth(174);ImGui::Combo("##choice", &it->second, list);
            auto end = ImGui::GetItemRectMax();auto start = ImGui::GetItemRectMin();auto* d = ImGui::GetWindowDrawList();
            d->AddTriangleFilled(ImVec2(end.x - 12, start.y + 4), ImVec2(end.x - 4, start.y + 4), ImVec2(end.x - 8, start.y + 11), Color(colors.config.text));
            ImGui::PopItemWidth();ImGui::SameLine();Label(label);ImGui::PopID();
        }
        void Input(const char* label) {
            auto& value = strings[Id(label)];ImGui::PushID(label);ImGui::PushItemWidth(174);
            ImGui::InputText("##input", value.data(), value.size());ImGui::PopItemWidth();
            ImGui::SameLine();Label(label);ImGui::PopID();
        }
        void Swatch(const char* label, ImVec4 initial = ImVec4(0, 1, 1, 1)) {
            auto [it, inserted] = swatches.try_emplace(Id(label), initial);(void)inserted;
            colors.ColorRow(label, it->second);
        }
        void Tabs(const char* id, std::initializer_list<const char*> labels, int& selected, float width, bool main = false) {
            bool childTabs = !main && width < 400;
            if (childTabs) { width = ImGui::GetWindowWidth();ImGui::SetCursorPosX(0); }
            ImGui::PushID(id);int i = 0;float each = width / static_cast<float>(labels.size());
            for (auto label : labels) {
                if (i)ImGui::SameLine(0, 0);auto p = ImGui::GetCursorScreenPos();
                ImGui::PushID(i);bool click = ImGui::InvisibleButton("tab", ImVec2(each, 32));
                if (click && !disabled)selected = i;auto* d = ImGui::GetWindowDrawList();
                d->AddRectFilledMultiColor(p, ImVec2(p.x + each, p.y + 32), Gray(39, 225), Gray(39, 225), Gray(26, 205), Gray(26, 205));
                if (ImGui::IsItemHovered())d->AddRectFilled(p, ImVec2(p.x + each, p.y + 32), Gray(60, 185));
                auto size = ImGui::CalcTextSize(label);
                d->AddText(ImVec2(p.x + (each - size.x) / 2, p.y + 10), selected == i ? AccentText() : Color(colors.config.text), label);
                if (!main)d->AddLine(ImVec2(p.x, p.y + 31), ImVec2(p.x + each, p.y + 31), selected == i ? Accent() : IM_COL32(95, 95, 95, 255), 2);
                ImGui::PopID();++i;
            }ImGui::PopID();if (childTabs)ImGui::SetCursorPosX(20);
        }
        void Panel(const char* id, float x, float y, float w = 300, float h = 370) {
            ImGui::SetCursorPos(ImVec2(x, y));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 2));
            ImGui::BeginChild(id, ImVec2(w, h), ImGuiChildFlags_AlwaysUseWindowPadding, (disabled ? ImGuiWindowFlags_NoInputs : 0));
            ImGui::PopStyleVar();
        }
        void EndPanel() { ImGui::EndChild(); }
        void List(const char* id, const std::vector<std::string>& names, int& value, float height, float width = 0) {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, (Light() ? ImVec4(.83f, .83f, .83f, 1) : ImVec4(.10f, .095f, .09f, 1)));
            ImGui::BeginChild(id, ImVec2(width, height), false, disabled ? ImGuiWindowFlags_NoInputs : 0);
            for (size_t i = 0;i < names.size();++i) {
                ImGui::PushID(static_cast<int>(i));auto p = ImGui::GetCursorScreenPos();float w = ImGui::GetContentRegionAvail().x;
                if (ImGui::InvisibleButton("row", ImVec2(w, 14)))value = static_cast<int>(i);
                auto* d = ImGui::GetWindowDrawList();if (value == static_cast<int>(i) || ImGui::IsItemHovered())d->AddRectFilledMultiColor(p, ImVec2(p.x + w, p.y + 16), Accent(.3f), Accent(.3f), Accent(), Accent());
                d->AddText(p, Color(colors.config.text), names[i].c_str());ImGui::PopID();
            }
            ImGui::EndChild();ImGui::PopStyleColor();
        }
        void AntiAim() {
            Panel("anti-left", 30, 120, 300, 270);
            Section("Anti-aim:");Combo("Pitch", "Off\0Up\0Down\0Random\0");Combo("Yaw", "Off\0Left\0Right\0Back\0");Combo("Yaw fake", "Off\0Left\0Right\0Back\0");Check("LBY breaker");EndPanel();
            Panel("anti-spread", 30, 412, 300, 78);Section("Spread resolver:");Check("Resolve all");EndPanel();
            Panel("anti-right", 363, 120, 297, 370);
            Section("Nospread resolver:");Check("Nospread resolver enable");Combo("Pitch", "Up\0Down\0Off\0");Combo("Yaw", "Self\0Left\0Right\0");Section("Self settings:");Combo("Base angle", "Fake body\0Real body\0");Combo("Resolve type", "Bruteforce multiply\0Off\0");Check("Jitter");Section("Bruteforce multiply:");Combo("Using", "Missed shots\0Always\0");Slider("Angle", 0, -180, 180);Section("");Check("Untrust", true);
            Section("CS2 aimbot:");BoundCheck("Enable [key not set]", cs2.aim);Slider("FOV##cs2", 6, 0, 40, "%.1f");Slider("Smooth##cs2", 3, 1, 20, "%.1f");Slider("Hit chance##cs2", 70, 0, 100);Slider("Randomize##cs2", 0.6f, 0, 5, "%.1f");BoundCheck("RCS##cs2", cs2.rcs);Slider("Recoil min##cs2", 40, 0, 100);Slider("Recoil max##cs2", 80, 0, 100);BoundCheck("Triggerbot [key not set]", cs2.trigger);BoundCheck("Auto fire##cs2", cs2.autoFire);BoundCheck("FOV circle##cs2", cs2.fovCircle);BoundCheck("Watermark##cs2", cs2.watermark);
            cs2.aimKey = keybinds[Id("Enable [key not set]")];cs2.triggerKey = keybinds[Id("Triggerbot [key not set]")];cs2.fov = numbers[Id("FOV##cs2")];cs2.smooth = numbers[Id("Smooth##cs2")];cs2.hitchance = numbers[Id("Hit chance##cs2")];cs2.randomize = numbers[Id("Randomize##cs2")];cs2.recoilMin = numbers[Id("Recoil min##cs2")];cs2.recoilMax = numbers[Id("Recoil max##cs2")];
            EndPanel();
        }
        void WeaponIcon(int index, ImVec2 p, ImU32 color) {
            // Local vector silhouettes; no game assets or game integration.
            auto* draw = ImGui::GetWindowDrawList();
            auto line = [&](float x, float y, float x2, float y2, float width) {draw->AddLine(ImVec2(p.x + x, p.y + y), ImVec2(p.x + x2, p.y + y2), color, width);};
            auto rect = [&](float x, float y, float w, float h) {draw->AddRectFilled(ImVec2(p.x + x, p.y + y), ImVec2(p.x + x + w, p.y + y + h), color);};
            if (index == 0) { rect(6, 4, 29, 4);rect(5, 7, 18, 3);line(10, 9, 7, 17, 5);rect(34, 5, 7, 2); }
            else if (index == 1) { rect(15, 7, 20, 4);line(2, 13, 16, 9, 5);line(20, 10, 18, 17, 4);line(27, 11, 30, 18, 4);line(33, 8, 48, 8, 2);rect(37, 5, 2, 4); }
            else if (index == 2) { rect(14, 7, 22, 4);line(2, 12, 15, 9, 4);line(21, 9, 19, 16, 3);line(34, 8, 51, 8, 2);rect(21, 2, 13, 3);rect(24, 5, 2, 3);line(39, 10, 41, 17, 1); }
            else if (index == 3) { rect(10, 6, 27, 7);line(0, 12, 13, 11, 4);rect(20, 3, 13, 2);line(25, 12, 25, 17, 4);rect(37, 8, 11, 2); }
            else if (index == 4) { rect(11, 5, 21, 7);line(0, 9, 13, 8, 4);rect(15, 2, 13, 2);line(16, 11, 14, 18, 3);line(27, 11, 28, 19, 4);line(31, 7, 47, 7, 2);rect(38, 4, 2, 5); }
            else { line(1, 13, 13, 8, 5);rect(12, 7, 34, 3);rect(16, 10, 13, 3);rect(44, 6, 7, 2); }
        }
        void NormalLegit() {
            ImGui::SetCursorPos(ImVec2(30, 160));
            const char* labels[]{ "Pistols","Rifles","Sniper rifles","SMGs","Machine guns","Shotguns" };
            for (int i = 0;i < 6;++i) {
                if (i)ImGui::SameLine(0, 0);auto pos = ImGui::GetCursorScreenPos();
                ImGui::PushID(i);if (ImGui::InvisibleButton("weapon-category", ImVec2(105, 34)) && !disabled)normalWeapon = i;
                WeaponIcon(i, ImVec2(pos.x + 28, pos.y + 7), i == normalWeapon ? Accent() : Color(colors.config.text));
                if (ImGui::IsItemHovered())ImGui::SetTooltip("%s", labels[i]);ImGui::PopID();
            }

            ImGui::PushID(normalWeapon);
            Panel("normal-aim", 30, 206, 300, 174);
            if (normalScrollRequest >= 0) { ImGui::SetScrollY(normalScrollRequest);normalScrollRequest = -1; }
            Section("Aimbot:");
            BoundCheck("Aimbot  [key not set]", cs2.aim);
            Checks({ "Always on","Friendlyfire","Silent aim","Jump check" });
            Section("Custom settings:");
            numbers[Id("FOV")] = normalFov[normalWeapon];Slider("FOV", 8, 0, 30, "%.2f");normalFov[normalWeapon] = numbers[Id("FOV")];
            Slider("Speed", 1, 0, 30, "%.2f");Slider("Speed - shooting", 1, 0, 30, "%.2f");
            Combo("Bone", "Auto\0Head\0Chest\0Stomach\0");
            Section("Non sticky aim:");Slider("Lock for X ms", 50, 0, 1000);Slider("Release after X ms", 100, 0, 1000);Slider("Reaction time (ms)", 25, 0, 500);

            cs2.aimKey = keybinds[Id("Aimbot  [key not set]")];
            cs2.fov = numbers[Id("FOV")];
            cs2.smooth = numbers[Id("Speed")];
            {
                int b = choices[Id("Bone")];
                cs2.bone = (b == 1) ? 0 : (b == 2 || b == 3) ? 1 : 2;
            }
            EndPanel();
            Panel("normal-rcs", 30, 392, 300, 98);
            Section("RCS:");Checks({ "RCS","RCS enabled always" });
            Slider("X min", 1, 0, 100, "%.2f");Slider("X max", 1, 0, 100, "%.2f");Slider("Y min", 1, 0, 100, "%.2f");Slider("Y max", 1, 0, 100, "%.2f");EndPanel();
            Panel("normal-trigger", 363, 206, 297, 284);
            Section("Triggerbot:");
            BoundCheck("Triggerbot##legit", cs2.trigger);
            Check("On key  [key not set]");
            Checks({ "Friendlyfire","Head","Chest","Stomach","Arms","Legs","Hit chance" });
            Slider("Hit chance", 100, 0, 100, "%.2f");
            Check("Delay");Slider("Delay", 250, 0, 1000);Slider("Min. damage", 30, 0, 100, "%.2f");Check("Backtrack");
            cs2.triggerKey = keybinds[Id("On key  [key not set]")];
            cs2.hitchance = numbers[Id("Hit chance")];
            EndPanel();ImGui::PopID();
        }
        void Legit() {
            ImGui::SetCursorPos(ImVec2(30, 120));Tabs("legit", { "Normal","Advanced" }, legit, 630);
            if (!legit) { NormalLegit();return; }
            Panel("legit-left", 30, 164, 300, 326);Tabs("legit-left-tabs", { "Global","Triggerbot","KnifeBot" }, legitLeft, 260);
            Section(legitLeft == 0 ? "Global settings:" : legitLeft == 1 ? "Triggerbot:" : "KnifeBot:");
            BoundCheck("Enable [key not set]##legit", cs2.aim);
            if (legitLeft == 0) {
                Check("Aim at teammates");
                BoundCheck("Auto fire##legit", cs2.autoFire);
                Checks({ "Aim on shot","Flash check","Jump check","Smoke check","Jump Enemy Check" });
                ImGui::Separator();
                BoundCheck("Draw Fov##legit", cs2.fovCircle);
                Checks({ "Show Target","Draw Silent Fov" });
                Check("Wall check", true);Check("Scope check", true);
                Checks({ "Auto pistol","Fast zoom","FaceIt","Entity aim" });
            }
            else if (legitLeft == 1) {
                Check("On key [key not set]##legit2");
                Slider("Delay", 100, 0, 500);
                Slider("Hitchance", 100, 0, 100);
                Check("Scope check");
                Combo("HitBox##trig", "Head\0Chest\0All\0");
                cs2.triggerKey = keybinds[Id("On key [key not set]##legit2")];
                cs2.hitchance = numbers[Id("Hitchance")];
                {
                    int b = choices[Id("HitBox##trig")];
                    cs2.bone = (b == 1) ? 1 : (b == 2) ? 2 : 0;
                }
            }
            else { Check("Team check");Slider("Distance", 60, 0, 100); }
            cs2.aimKey = keybinds[Id("Enable [key not set]##legit")];
            EndPanel();
            Panel("legit-right", 363, 164, 297, 326);Tabs("legit-right-tabs", { "Basic","Rcs","pSilent","Delays" }, legitRight, 257);
            if (legitRight == 0) {
                Section("Aim settings:");
                Slider("Fov##legit", 8, 0, 30, "%.2f");
                Slider("Smooth##legit", 3, 1, 30, "%.2f");
                Check("Nearest");
                Combo("HitBox##legit", "Head\0Chest\0Nearest\0");
                Slider("Hit chance##legit", 100, 0, 100);
                cs2.fov = numbers[Id("Fov##legit")];
                cs2.smooth = numbers[Id("Smooth##legit")];
                cs2.hitchance = numbers[Id("Hit chance##legit")];
                cs2.bone = choices[Id("HitBox##legit")];
            }
            else if (legitRight == 1) { Check("Enable");Slider("X", 100, 0, 100);Slider("Y", 100, 0, 100);Slider("Start bullet", 1, 0, 10); }
            else if (legitRight == 2) { ImGui::Dummy(ImVec2(0, 12));Check("Silent");Slider("%chance", 100, 0, 100);Slider("Fov", 20, 0, 30, "%.2f");Check("Nearest");Combo("HitBox", "Head\0Chest\0Pelvis\0");Slider("Start bullet", 0, 0, 10);Slider("End bullet", 1, 0, 10); }
            else { Section("Delays:");Slider("First shot", 100, 0, 500);Slider("Target switch", 100, 0, 500); }
            ImGui::SetCursorPosY(290);if (ImGui::Button("Weapon config", ImVec2(257, 18)))weaponWindow = !weaponWindow;
            EndPanel();
        }
        void DrawPlayerPreview() {
            auto origin = ImGui::GetWindowPos();auto* d = ImGui::GetWindowDrawList();
            ImVec2 a(origin.x + 70, origin.y + 40), b(a.x + 183, a.y + 244);
            if (playerTexture)d->AddImage(playerTexture, a, b, ImVec2(0, 0), ImVec2(1, 1), preview.chams ? Color(preview.color) : IM_COL32(255, 255, 255, 255));
            ImVec2 lo(a.x + 16, a.y + 4), hi(b.x - 12, b.y - 4);
            if (preview.glow)for (int n = 5;n > 0;--n) { auto c = preview.color;c.w = .08f;d->AddRect(ImVec2(lo.x - n, lo.y - n), ImVec2(hi.x + n, hi.y + n), Color(c), 0, 3.f, ImDrawFlags_RoundCornersAll); }
            if (preview.boxes) { d->AddRect(ImVec2(lo.x - 1, lo.y - 1), ImVec2(hi.x + 1, hi.y + 1), IM_COL32(0, 0, 0, 255));d->AddRect(lo, hi, Color(preview.color)); }
            if (preview.health) { d->AddRectFilled(ImVec2(lo.x - 9, lo.y), ImVec2(lo.x - 4, hi.y), IM_COL32(12, 12, 12, 255));d->AddRectFilled(ImVec2(lo.x - 8, hi.y - (hi.y - lo.y) * .75f), ImVec2(lo.x - 5, hi.y), IM_COL32(70, 220, 65, 255));d->AddText(ImVec2(lo.x - 29, lo.y + 48), Color(colors.config.text), "75"); }
            if (preview.armour) { d->AddRectFilled(ImVec2(hi.x + 4, lo.y), ImVec2(hi.x + 9, hi.y), IM_COL32(12, 12, 12, 255));d->AddRectFilled(ImVec2(hi.x + 5, lo.y + 20), ImVec2(hi.x + 8, hi.y), IM_COL32(65, 160, 255, 255)); }
            if (preview.names)d->AddText(ImVec2(lo.x + 38, lo.y - 17), Color(colors.config.text), "Preview player");
            if (preview.weapon)d->AddText(ImVec2(lo.x + 53, hi.y + 7), Color(colors.config.text), "AK-47");
            if (preview.head)d->AddRect(ImVec2(a.x + 84, a.y + 2), ImVec2(a.x + 116, a.y + 38), Color(preview.color));
            if (preview.snaplines)d->AddLine(ImVec2(origin.x + 148, origin.y + 306), ImVec2((lo.x + hi.x) / 2, hi.y), Color(preview.color));
            if (preview.arrows)d->AddTriangleFilled(ImVec2(lo.x - 38, lo.y + 95), ImVec2(lo.x - 24, lo.y + 88), ImVec2(lo.x - 24, lo.y + 102), Color(preview.color));
            if (preview.sound)d->AddCircle(ImVec2((lo.x + hi.x) / 2, hi.y - 8), 48, Color(preview.color), 48);
            if (preview.crosshair) { ImVec2 c((lo.x + hi.x) / 2, (lo.y + hi.y) / 2);d->AddLine(ImVec2(c.x - 7, c.y), ImVec2(c.x + 7, c.y), Color(preview.color));d->AddLine(ImVec2(c.x, c.y - 7), ImVec2(c.x, c.y + 7), Color(preview.color)); }
        }
        void Visuals() {
            ImGui::SetCursorPos(ImVec2(30, 120));Tabs("visuals", { "ESP","Other" }, visuals, 630);
            Panel("visuals-left", 30, 180, 300, 310);
            if (!visuals) {
                Tabs("esp", { "ESP","GLOW","CHAMS" }, esp, 260);Section("Players:");BoundCheck("Team check", cs2.team);
                if (esp == 0) {
                    BoundCheck("Boxes", preview.boxes);BoundCheck("Names", preview.names);BoundCheck("Health", preview.health);BoundCheck("Armour", preview.armour);BoundCheck("Weapon", preview.weapon);BoundCheck("Snaplines", preview.snaplines);BoundCheck("POV arrows", preview.arrows);Slider("Size", 259, 0, 500);BoundCheck("Sound ESP", preview.sound);BoundCheck("Head box", preview.head);Section("Other:");ImGui::TextUnformatted("Spread crosshair");Combo("", "Off\0Cross\0Circle\0");BoundCheck("Crosshair", preview.crosshair);Check("Dropped weapons", true);
                    Section("CS2:");BoundCheck("Ammo##cs2", cs2.ammo);Slider("Max dist##cs2", 300, 50, 1000);BoundCheck("Bomb info##cs2", cs2.bombinfo);cs2.dist = numbers[Id("Max dist##cs2")];
                }
                else { BoundCheck("Enable", esp == 1 ? preview.glow : preview.chams);Combo("Material", "Flat\0Textured\0Metallic\0");Check("Visible only");colors.ColorRow("Visible", preview.color);Swatch("Occluded", ImVec4(1, 0, 0, 1)); }
            }
            else {
                Section("Antiaims:");Check("Fake angle chams");Section("Lag comp:");Checks({ "Backtrack skelt","Backtrack chams" });Section("Colors:");Check("Colored walls");Swatch("Bullet tracer", ImVec4(0, 1, 0, 1));Swatch("Bullet tracer others", ImVec4(1, 0, 0, 1));
                if (ImGui::Button("Set light mode", ImVec2(126, 16)))colors.Light();ImGui::SameLine();if (ImGui::Button("Set dark mode", ImVec2(126, 16)))colors.Dark();
                colors.ColorRow("Theme color", colors.config.accent);colors.ColorRow("BG color", colors.config.windowBg);colors.ColorRow("BG Child color", colors.config.panelBg);colors.ColorRow("Text color", colors.config.text);
            }
            EndPanel();Panel("visuals-right", 363, 180, 297, 310);
            if (!visuals) { Section("Preview:");DrawPlayerPreview(); }
            else { Section("Removals:");Checks({ "No visual recoil","No hands","No smoke","No flash","No scope","No scope zoom" });Check("No postprocessing", true);Checks({ "Hide from OBS","Watermark","Corner info" });Section("Other:");Check("Nightmode");Slider("Viewmodel fov", 75, 0, 180);Slider("Player fov", 100, 0, 180);Check("Hitmarker", true);Slider("Hitmarker size", 5, 1, 20);Combo("Hitmark sound", "Skeet sound\0COD sound\0Off\0");Check("Bullet Tracer");Check("Grenade prediction", true);Check("Damage indicator"); }
            EndPanel();
        }
        void Misc() {
            Panel("misc-left", 30, 120);Tabs("misc", { "MISC #1","MISC #2","Rank changer" }, misc, 260);
            if (misc == 0) { Section("Misc:");BoundCheck("Bunnyhop", cs2.bhop);Checks({ "Auto strafe","Edge jump [v]" });Check("Show competitive ranks", true);Check("Third person [key not set]");Check("Block bot [c]", true);Check("Grief nade [v]", true);BoundCheck("Spectator list", spectators);Check("Event log");Check("Player info box", true);Check("Team check [TAB]");Check("Grief box [TAB]", true);Slider("Thirdperson dist.", 150, 0, 500, "%.3f");Check("Overwatch revealer");Check("Run Boost [MOUSE5]", true);Section("Fakelag:");Checks({ "Enable","Adaptive" });Combo("Type", "Always\0Moving\0");Slider("Choke", 0, 0, 16);Check("Fake walk [key not set]"); }
            else if (misc == 1) { Section("ClanTag changer:");Combo("Type", "Disabled\0Custom\0Animated\0");Input("Clantag");Combo("Animation type", "Gamesense\0Static\0");Section("Name changer:");Combo("Type##name", "Name stealer\0Custom\0");Input("Name");Combo("Animation type##name", "Static\0Animated\0");Combo("Name stealer type", "Enemies only\0All\0");Section("Chat spam:");Check("Spam!");Input("Message");Check("Trash talk"); }
            else { Section("Rank changer:");Combo("Rank", "Off\0Silver I\0Gold Nova I\0Global Elite\0");Slider("Wins", 0, 0, 999);Slider("Level", 1, 1, 40);ImGui::Button("Update", ImVec2(260, 18)); }
            EndPanel();Panel("misc-right", 363, 120, 297);
            Section("Grenade helper:");Check("Enable");Combo("Grenade", "Smoke\0Flash\0HE\0Molotov\0");Check("Show locations");Check("Show trajectory");Slider("Distance", 1000, 0, 5000);
            Section("CS2 match:");BoundCheck("Show info##cs2", cs2.showinfo);
            EndPanel();
        }
        void Skins() {
            Panel("skin-left", 30, 120);Section("Select weapon:");
            List("weapons", { "Knife","Glove","AK-47","AUG","AWP","CZ75 Auto","Desert Eagle","Dual Berettas","FAMAS","Glock-18","M4A4","USP-S" }, weapon, 125);
            ImGui::Button("Update");Section("Preview:");
            ImGui::SetCursorPos(ImVec2(45, 210));
            if (weapon == 1 && gloveTexture)ImGui::Image(gloveTexture, ImVec2(190, 135));
            else if (knifeTexture)ImGui::Image(knifeTexture, ImVec2(195, 145));
            else ImGui::Dummy(ImVec2(195, 145));
            EndPanel();Panel("skin-right", 363, 120, 297);Tabs("skin", { "Skins","Stickers" }, skin, 257);
            Section("Weapon settings:");Slider("Seed", 55, 0, 1000);Slider("Stat trak", 0, 0, 1000);Slider("Wear", .000001f, 0, 1, "%.6f");
            Combo(weapon == 1 ? "Glove" : "Knife", weapon == 1 ? "Specialist\0Sport\0Driver\0" : "M9 Bayonet\0Karambit\0Bayonet\0");Input("Name tag");
            Section(skin ? "Stickers:" : "Skin:");Check("Filter by weapon", true);Input("##filter");
            if (skin)List("sticker-list", { "None","Crown","Dragon","Star","Team logo" }, skinChoice, 120);
            else if (weapon == 1)List("skin-list", { "Crimson Kimono","Crimson Web","Emerald Web","Fade","Forest DDPAT","Foundation","Mogul" }, skinChoice, 120);
            else List("skin-list", { "Rust Coat","Safari Mesh","Scorched","Slaughter","Stained","Tiger Tooth","Ultraviolet","Urban Masked" }, skinChoice, 120);
            ImGui::Button("Set");EndPanel();
        }
        void Config() {
            Panel("config-left", 30, 120);Section("Config Manager:");
            auto& name = strings["profile-name"];ImGui::PushItemWidth(132);ImGui::InputText("##profile-name", name.data(), name.size());ImGui::PopItemWidth();
            ImGui::SameLine();if (ImGui::Button("Refresh", ImVec2(130, 16)))profile = std::clamp(profile, 0, static_cast<int>(profiles.size()) - 1);
            if (ImGui::Button("Add & Save", ImVec2(132, 16)) && name[0]) { profiles.emplace_back(name.data());profile = static_cast<int>(profiles.size()) - 1;savedChecks[profiles.back()] = checks;savedNumbers[profiles.back()] = numbers;savedChoices[profiles.back()] = choices;savedKeybinds[profiles.back()] = keybinds; }
            if (ImGui::Button("Delete selected config", ImVec2(132, 16)) && profiles.size() > 1) { profiles.erase(profiles.begin() + profile);profile = std::clamp(profile, 0, static_cast<int>(profiles.size()) - 1); }
            ImGui::SetCursorPos(ImVec2(160, 43));List("profiles", profiles, profile, 156, 130);
            ImGui::SetCursorPos(ImVec2(160, 202));
            if (ImGui::Button("Save", ImVec2(62, 16))) { savedChecks[profiles[profile]] = checks;savedNumbers[profiles[profile]] = numbers;savedChoices[profiles[profile]] = choices;savedKeybinds[profiles[profile]] = keybinds; }
            ImGui::SameLine();if (ImGui::Button("Load", ImVec2(62, 16)) && savedChecks.contains(profiles[profile])) { checks = savedChecks[profiles[profile]];numbers = savedNumbers[profiles[profile]];choices = savedChoices[profiles[profile]];keybinds = savedKeybinds[profiles[profile]]; }
            EndPanel();Panel("config-right", 363, 120, 297);Section("Global configs:");EndPanel();
        }
        struct Spectator { std::string name; bool thirdPerson = false; ImTextureID avatar = 0; };
        std::vector<Spectator> spectatorRows;
        // Compact reference: avatar left, nickname above viewing mode; no column headings.
        void Spectators() {
            const float width = 224, rowHeight = 46, headerHeight = 22;
            const float height = headerHeight + 4 + rowHeight * static_cast<float>((std::max)(size_t{ 1 }, spectatorRows.size()));
            ImGui::SetNextWindowPos(ImVec2(740, 36), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.f);
            ImGui::Begin("##spectator-list", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            auto p = ImGui::GetWindowPos();auto* d = ImGui::GetWindowDrawList();
            d->PushClipRect(p, ImVec2(p.x + width, p.y + height), false);
            d->AddRectFilled(p, ImVec2(p.x + width, p.y + height), Color(colors.config.windowBg));
            auto header = colors.config.accent;
            d->AddRectFilled(p, ImVec2(p.x + width, p.y + headerHeight), Color(header));
            d->AddRect(p, ImVec2(p.x + width, p.y + height), Gray(65, 145));
            d->AddLine(ImVec2(p.x, p.y + headerHeight), ImVec2(p.x + width, p.y + headerHeight), Gray(65, 145));
            const char* title = "Spectator list";const ImU32 text = Color(colors.config.text);
            const auto titleSize = ImGui::CalcTextSize(title);
            d->AddText(ImVec2(p.x + (width - titleSize.x) / 2, p.y + 4), text, title);
            for (size_t i = 0;i < spectatorRows.size();++i) {
                const auto& row = spectatorRows[i];const float y = p.y + headerHeight + 6 + rowHeight * static_cast<float>(i);
                const ImVec2 avatar(p.x + 16, y + 2), avatarEnd(p.x + 48, y + 34);
                if (row.avatar)d->AddImage(row.avatar, avatar, avatarEnd);
                else { d->AddRectFilled(avatar, avatarEnd, Color(colors.config.panelBg), 3.f);d->AddText(ImVec2(avatar.x + 12, avatar.y + 8), text, "?"); }
                d->PushClipRect(ImVec2(p.x + 60, y), ImVec2(p.x + width - 8, y + rowHeight), true);
                d->AddText(ImVec2(p.x + 64, y + 1), text, row.name.c_str());
                d->AddText(ImVec2(p.x + 64, y + 16), text, row.thirdPerson ? "3rd person" : "1st person");d->PopClipRect();
                if (i + 1 < spectatorRows.size())d->AddLine(ImVec2(p.x + 10, y + 39), ImVec2(p.x + width - 10, y + 39), Gray(65, 180));
            }
            d->PopClipRect();ImGui::End();
        }
        void Club() {
            ImGui::SetNextWindowSize(ImVec2(640, 550), ImGuiCond_Always);
            ImGui::SetNextWindowPos(ImVec2(100, 60), ImGuiCond_FirstUseEver);
            ImGui::PushStyleColor(ImGuiCol_TitleBg, colors.config.accent);ImGui::PushStyleColor(ImGuiCol_TitleBgActive, colors.config.accent);
            ImGui::Begin("##members-club", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            auto origin = ImGui::GetWindowPos();auto* d = ImGui::GetWindowDrawList();
            d->AddRectFilled(origin, ImVec2(origin.x + 640, origin.y + 50), Accent(.55f));
            const char* title = "GETREKT MEMBERS' CLUB";
            d->AddText(ImVec2(origin.x + (640 - ImGui::CalcTextSize(title).x) / 2, origin.y + 19), Color(colors.config.text), title);
            ImGui::SetCursorPos(ImVec2(28, 80));
            ImGui::BeginChild("club-content", ImVec2(584, 442), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            ImGui::TextUnformatted("Users online: demo");
            ImGui::TextUnformatted("Content will refresh in: 00:04");
            if (ImGui::Button("Force update")) {};
            Check("Replace friends with GETREKT' MEMBERS");
            ImGui::TextColored(ImVec4(.8f, .18f, .15f, 1), "More features for GETREKT CLUB like: messages, info about");
            ImGui::TextColored(ImVec4(.8f, .18f, .15f, 1), "current connected server, profile picture and much more coming soon!");
            Section("Change log:");
            ImGui::BeginChild("historical-changelog", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
            struct Row { const char* version;const char* type;const char* text; };
            const Row rows[] = {
                { "Version 1.4.6","ADD","Player info box" },{ "","ADD","Grief box" },{ "","ADD","Run boost" },{ "","ADD","Overwatch revealer" },{ "","ADD","Clantag stealer" },{ "","ADD","Event log" },{ "","ADD","Spread cross" },{ "","FIX","Name stealer \"GOTV\" ignored" },
                { "Version 1.4.5","UPDATE","CSGO update" },{ "","ADD","Name stealer" },{ "","ADD","98 Nade" },{ "","ADD","Name/Clan changer ADD gamesense animation style" },{ "","CHANGED","Name/Clan changer synced" },{ "","CHANGED","Clantag animation \"___\" chars changed" },{ "","FIX","Skinchanger filter out correctly M4A4 skins" } };
            ImGui::Columns(3, "change-columns", false);ImGui::SetColumnOffset(1, 100);ImGui::SetColumnOffset(2, 180);
            for (const auto& row : rows) {
                ImGui::TextUnformatted(row.version);ImGui::NextColumn();auto p = ImGui::GetCursorScreenPos();
                ImU32 color = strcmp(row.type, "ADD") == 0 ? IM_COL32(35, 125, 40, 255) : strcmp(row.type, "FIX") == 0 ? IM_COL32(35, 100, 235, 255) : strcmp(row.type, "UPDATE") == 0 ? IM_COL32(155, 45, 200, 255) : IM_COL32(255, 125, 55, 255);
                ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + 76, p.y + 19), color);
                ImGui::GetWindowDrawList()->AddText(ImVec2(p.x + 5, p.y + 2), IM_COL32(255, 255, 255, 255), row.type);ImGui::Dummy(ImVec2(76, 19));ImGui::NextColumn();ImGui::TextUnformatted(row.text);ImGui::NextColumn();
            }
            ImGui::Columns(1);ImGui::EndChild();ImGui::EndChild();ImGui::End();ImGui::PopStyleColor(2);
        }
        void Draw(bool panic) {
            disabled = panic;colors.Apply();auto& style = ImGui::GetStyle();style.ItemSpacing = ImVec2(4, 2);style.FramePadding = ImVec2(2, 1);style.GrabMinSize = 9;style.ScrollbarSize = 6;
            style.Colors[ImGuiCol_FrameBg] = Light() ? ImVec4(.78f, .78f, .78f, 1) : ImVec4(.06f, .06f, .06f, 1);
            ImGui::SetNextWindowSize(ImVec2(690, 520), ImGuiCond_Always);ImGui::SetNextWindowPos(ImVec2(24, 36), ImGuiCond_FirstUseEver);
            colors.BeginMenu("##video-reference", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | (panic ? ImGuiWindowFlags_NoInputs : 0));
            auto p = ImGui::GetWindowPos();auto* d = ImGui::GetWindowDrawList();d->PushClipRect(p, ImVec2(p.x + 690, p.y + 520), false);
            d->AddRectFilled(ImVec2(p.x, p.y + 92), ImVec2(p.x + 690, p.y + 520), Color(colors.config.windowBg));
            d->AddRect(ImVec2(p.x + 4, p.y + 95), ImVec2(p.x + 686, p.y + 516), Gray(5, 165));
            d->AddRect(ImVec2(p.x + 1, p.y + 92), ImVec2(p.x + 689, p.y + 519), IM_COL32(65, 63, 59, 255));
            d->AddRectFilled(ImVec2(p.x, p.y + 16), ImVec2(p.x + 690, p.y + 58), Accent(.55f));
            d->AddRectFilledMultiColor(ImVec2(p.x, p.y + 58), ImVec2(p.x + 690, p.y + 92), Gray(47, 225), Gray(47, 225), Gray(30, 205), Gray(30, 205));
            const float captionWidth = ImGui::CalcTextSize("GetRekt Club").x + 6;
            d->AddRectFilled(p, ImVec2(p.x + captionWidth, p.y + 16), Accent());d->AddText(ImVec2(p.x + 3, p.y + 2), IM_COL32(255, 255, 255, 255), "GetRekt Club");
            auto title = ImGui::CalcTextSize("GetRekt");d->AddText(ImVec2(p.x + (690 - title.x) / 2, p.y + 31), IM_COL32(255, 255, 255, 255), "GetRekt");d->PopClipRect();
            ImGui::SetCursorPos(ImVec2(0, 0));if (ImGui::InvisibleButton("caption", ImVec2(captionWidth, 16)))clubWindow = !clubWindow;
            ImGui::SetCursorPos(ImVec2(0, 16));ImGui::InvisibleButton("drag", ImVec2(690, 42));if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) { auto delta = ImGui::GetIO().MouseDelta;ImGui::SetWindowPos(ImVec2(std::clamp(p.x + delta.x, 0.f, (std::max)(0.f, ImGui::GetIO().DisplaySize.x - 690)), std::clamp(p.y + delta.y, 0.f, (std::max)(0.f, ImGui::GetIO().DisplaySize.y - 520)))); }
            ImGui::SetCursorPos(ImVec2(0, 58));Tabs("main", { "ANTI-AIM","LEGIT","VISUALS","MISC","SKIN-CHANGER","CONFIG" }, page, 690, true);
            if (page == 0)AntiAim();else if (page == 1)Legit();else if (page == 2)Visuals();else if (page == 3)Misc();else if (page == 4)Skins();else Config();

            ImGui::End();
            if (clubWindow && !panic)Club();
            if (spectators && !panic)Spectators();
            if (page == 1 && legit == 1 && weaponWindow && !panic) { ImGui::SetNextWindowPos(ImVec2(p.x + 704, p.y + 92), ImGuiCond_Always);ImGui::SetNextWindowSize(ImVec2(420, 428), ImGuiCond_Always);ImGui::Begin("Weapon config", &weaponWindow, ImGuiWindowFlags_NoResize);Section("Weapon config:");Combo("", "Custom sections\0Default\0");Input("##section-name");ImGui::Button("Rename");ImGui::SameLine();ImGui::Button("Remove");static int category = 1, item = 0;List("categories", { "RIFLES","PISTOLS","SEMI-RIFLES","SHOTGUNS","SNIPER RIFLES" }, category, 280, 148);ImGui::SameLine();List("items", { "Deagle","Elite","Five-Seven","Glock-18","USP-S","CZ-75","R8 Revolver","Tec-9","P250","P2000" }, item, 280, 148);ImGui::End(); }
        }
    };
}