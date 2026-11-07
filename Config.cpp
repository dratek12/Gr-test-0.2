#include "Config.h"
#include "Global.h"

static void Path(wchar_t* out) {
    GetModuleFileNameW(G::Module, out, MAX_PATH);
    wchar_t* s = wcsrchr(out, L'\\');
    if (s) *(s + 1) = 0;
    wcscat_s(out, MAX_PATH, L"cs2esp.ini");
}

void SaveConfig() {
    wchar_t p[MAX_PATH]{};
    Path(p);
    FILE* f = nullptr;
    if (_wfopen_s(&f, p, L"w") || !f) return;
    auto& s = G::Ui.state;
    fprintf(f, "aim=%d\nrcs=%d\nammo=%d\nteam=%d\nbhop=%d\ntrigger=%d\n",
        s.cs2.aim, s.cs2.rcs, s.cs2.ammo, s.cs2.team, s.cs2.bhop, s.cs2.trigger);
    fprintf(f, "fov=%.3f\nsmooth=%.3f\nhitchance=%.3f\nrandomize=%.3f\nrecoilMin=%.3f\nrecoilMax=%.3f\n",
        s.cs2.fov, s.cs2.smooth, s.cs2.hitchance, s.cs2.randomize, s.cs2.recoilMin, s.cs2.recoilMax);
    fprintf(f, "boxes=%d\nnames=%d\nhealth=%d\nsnaplines=%d\nhead=%d\n",
        s.preview.boxes, s.preview.names, s.preview.health, s.preview.snaplines, s.preview.head);
    fclose(f);
    G::Log("config ulozen");
}

void LoadConfig() {
    wchar_t p[MAX_PATH]{};
    Path(p);
    FILE* f = nullptr;
    if (_wfopen_s(&f, p, L"r") || !f) return;
    auto& s = G::Ui.state;
    char line[128];
    while (fgets(line, sizeof line, f)) {
        char key[64]{}, val[64]{};
        if (sscanf_s(line, "%63[^=]=%63s", key, 64u, val, 64u) != 2) continue;
        int i = atoi(val);
        float fl = static_cast<float>(atof(val));
        if (!strcmp(key, "aim")) s.cs2.aim = i;
        else if (!strcmp(key, "rcs")) s.cs2.rcs = i;
        else if (!strcmp(key, "ammo")) s.cs2.ammo = i;
        else if (!strcmp(key, "team")) s.cs2.team = i;
        else if (!strcmp(key, "bhop")) s.cs2.bhop = i;
        else if (!strcmp(key, "trigger")) s.cs2.trigger = i;
        else if (!strcmp(key, "fov")) s.cs2.fov = fl;
        else if (!strcmp(key, "smooth")) s.cs2.smooth = fl;
        else if (!strcmp(key, "hitchance")) s.cs2.hitchance = fl;
        else if (!strcmp(key, "randomize")) s.cs2.randomize = fl;
        else if (!strcmp(key, "recoilMin")) s.cs2.recoilMin = fl;
        else if (!strcmp(key, "recoilMax")) s.cs2.recoilMax = fl;
        else if (!strcmp(key, "boxes")) s.preview.boxes = i;
        else if (!strcmp(key, "names")) s.preview.names = i;
        else if (!strcmp(key, "health")) s.preview.health = i;
        else if (!strcmp(key, "snaplines")) s.preview.snaplines = i;
        else if (!strcmp(key, "head")) s.preview.head = i;
    }
    fclose(f);
    G::Log("config nacten");
}