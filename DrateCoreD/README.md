# GetRekt ImGui Kit — Modern 1.93.0

Přenosná verze UI z GetRektLab 1.5 pro vlastní C++ projekt. Uživatelské rozhraní již používalo Dear ImGui; tento balíček ho odděluje od LANO launcheru, C#, DLL ABI a simulačního hostu. Obsahuje lokální ovladače a náhledy, nikoli implementace funkcí hry uvedených v popiscích.

## Rychlý start

- `bin/GetRektUIDemo.exe`: samostatná Win32/DX11 ukázka; Insert skryje/zobrazí menu. Nevyžaduje .NET ani LANO.
- `PROMPT-PRO-DALSI-CHAT.md`: celý text přilož k ZIPu v dalším chatu.
- `include/getrekt/GetRektUI.hpp`: veřejný vstup modulu.
- `examples/win32_dx11.cpp`: kompletní příklad vytvoření vlastního okna, zařízení, textur a předávání událostí.

Předkompilovaná ukázka je Windows x64; může vyžadovat nainstalovaný MSVC C++ runtime. Zdrojový modul vyžaduje C++20 a **Dear ImGui 1.93.0**, které je přibalené z oficiálního tagu https://github.com/ocornut/imgui/releases/tag/v1.93.0. Používá samostatné aktuální backendy `imgui_impl_win32` a `imgui_impl_dx11`. Ověřeno s 1.93.0; kompatibilita s budoucími verzemi není automaticky zaručena. Staré 1.51 už není podporované.

## Sestavení

Z terminálu s CMake a MSVC / Windows SDK:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
./build/Release/GetRektUIDemo.exe
```

`GetRektUIDemo.exe --smoke` provede osm snímků přes softwarové DX11 bez zobrazení okna. `getrekt_smoke` ověří UI bez rendereru. Závislosti jsou přibalené, CMake nic nestahuje.

## Zapojení do vlastního projektu

Pokud projekt dosud nemá ImGui:

```cmake
set(GETREKT_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(GETREKT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(external/GetRekt-ImGui-Kit-Modern)
target_link_libraries(MyApp PRIVATE getrekt_ui)
```

`getrekt_ui` připojí ImGui core, ale aplikace stále musí inicializovat platformní a rendererový backend. V případě stejné existující verze ImGui 1.93.0 vypni `GETREKT_BUNDLED_IMGUI` a připoj svůj existující ImGui target k `getrekt_ui` přes `target_link_libraries(getrekt_ui INTERFACE MyImGui)`. Nikdy nelinkuj dvě kopie ImGui. Pokud má cílový projekt jinou verzi, sjednoť ji s přibalenou 1.93.0 nebo proveď konkrétní kompatibilitní kontrolu. Žádné změny do vendor zdrojáků nejsou potřeba; vlastní picker byl přesunut do `detail/ReferenceColorPicker.hpp`.

Smyčka moderního backendu má pořadí:
```cpp
ImGui_ImplDX11_NewFrame();
ImGui_ImplWin32_NewFrame();
ImGui::NewFrame();
menu.draw();
ImGui::Render();
ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
```
Před inicializací backendů zavolej `IMGUI_CHECKVERSION(); ImGui::CreateContext();`. Při ukončení oba backendy vypni a zavolej `ImGui::DestroyContext();`. Nové oficiální backendy obsluhují také aktualizace fontových textur. Nepoužívej zde starý kombinovaný backend z původního ZIPu.


```cpp
#include <getrekt/GetRektUI.hpp>
getrekt::UI menu; // jednou, držet po celý život UI

// V každém snímku po NewFrame a před Render:
menu.draw();
```

Host vlastní ImGui kontext, zařízení, okno, textury a smyčku událostí. Modul nic z toho nevytváří ani neuvolňuje. Volat pouze na UI vlákně se správným aktuálním kontextem; jedna instance na kontext (okna používají pevná ID). `draw()` obnoví původní ImGuiStyle, aby nezměnil zbytek aplikace. Pro stejný vzhled načti Tahoma 13 px; font není součástí balíčku.

## Vstupy a keybindy

Kliknutí na `[key not set]` nebo existující vazbu aktivuje zachytávání. Escape smaže; Mouse 1–5 jsou podporované. Výchozí Insert/Delete, F10 a systémové klávesy jsou rezervované. Modul sám Insert/Delete nezpracovává jako menu/panic — to dělá host. Ukázka implementuje Insert; panic lze ovládat `setPanic()`.

Na Windows volej `getrekt::Win32Input(menu, message, wParam, lParam)` **před** ImGui backendovým handlerem a před aplikačními zkratkami. Pokud vrátí true, událost byla spotřebována zachytáváním vazby. Kompletní pořadí je v ukázce. Adaptér zpracuje ztrátu/získání focusu; ostatní ImGui vstupy předá běžný backend.

Na jiné platformě používej `keyEvent(code, down)`, `setFocused()`, `setVisible()` a `setPanic()`. Kódy jsou vlastní kompatibilní 8bitové hodnoty ve stylu Windows VK, **ne ImGuiKey ani scancode**: A–Z = 65–90, číslice = 48–57, Escape = 27, Mouse 1/2/3/4/5 = 1/2/4/5/6, Insert = 45, Delete = 46, F1–F24 = 112–135. Předej down i up a při ztrátě focusu zavolej `setFocused(false)`. Platformní podporu mimo Windows zatím netestováno.

Vazby přepínají lokální checkboxy pouze při skrytém menu, aktivním focusu a vypnutém panic. Opakovaný keydown nepřepíná opakovaně. Stejná klávesa může být přiřazená více položkám. Stav a profily jsou pouze v paměti.

## Barvy, data a textury

```cpp
menu.state.colors.config.text = ImVec4(0.9f, 0.9f, 0.9f, 1);
menu.state.colors.config.accent = ImVec4(1, 0.48f, 0.045f, 1);
menu.state.colors.Light(); // nebo Dark(); nastaví celé barevné schéma
menu.state.spectators = true;
menu.state.spectatorRows = {{"Ukazkovy nick", false, myAvatarTexture}};
menu.setPreviewTextures(myPlayerTexture, myKnifeTexture, myGloveTexture);
```

U spectator položky `false` znamená 1st person, `true` 3rd person. Textury jsou zapůjčené `ImTextureID`; host je musí držet živé až do vykreslení. V přibaleném DX11 backendu ulož `ID3D11ShaderResourceView*` přes `(ImTextureID)(intptr_t)srv`. Výchozí moderní ImTextureID je 64bitové číslo, prázdná textura je `0`, nikoli nullptr. ImGui následně používá ImTextureRef při kreslení. Nulový avatar ukáže zástupný znak. Seznam je standardně prázdný a žádná Steam data nestahuje. Volitelný `assets/preview_assets.h` obsahuje statické RGBA výřezy z uživatelem dodaného videa; rendererově nezávislé UI je nevyžaduje, ukázka je načítá.

Normal má šest kategorií, samostatné rolování a respektuje Text color. Change log má pevnou velikost a vnitřní scroll. Spectator list má pevnou šířku 224 px a výšku odvozenou od počtu položek. Podle opravené reference má pozadí podle barev menu, avatar vlevo a nick s režimem pohledu pod ním vpravo. Nemá sloupce Name/Mode. Tělo používá BG color (windowBg), hlavička Theme color (accent), text Text color a chybějící avatar BG Child color (panelBg). Změny barev i světlý/tmavý režim se projeví automaticky; při chybějícím avataru zůstává otazník. Obsah klubového panelu a historický changelog jsou ilustrační, ne živá služba. Na velmi dlouhém seznamu není implementována virtualizace ani omezení na výšku displeje.

## Úpravy a omezení API

Rozložení a popisky upravuj v `detail/VisualReplica.hpp`; společný styl v `detail/GetRektTheme.h`. `UI::state` je záměrně dostupný pro integraci, není to stabilní serializační ABI. `checks`, `numbers`, `choices` a `keybinds` jsou indexované ImGui ID převedenými na řetězce; nespoléhej na ně jako na trvalé aplikační identifikátory. Před propojením s doménovou logikou nebo uložením na disk zaveď pojmenovaný datový model a verzi schématu. Současné Config profily nepřežijí ukončení aplikace.

## Licence a původ

Dear ImGui: viz `LICENSE-IMGUI.txt`. ThemeKit a reference pocházejí z uživatelem dodaných podkladů; tímto balíčkem se k nim neuděluje nová licence. Ověř vlastní oprávnění při další distribuci grafických podkladů. Historické názvy funkcí jsou pouze UI popisky.

## Ověření tohoto balíčku

Samostatný build MSVC x64; renderer-free smoke test všech hlavních stránek, zachování stylu hosta a Text color, keybindů a focus/repeat pravidel; osm snímků samostatné DX11/WARP ukázky. Debug build běží i se zapnutými ImGui assertions; test obsahuje skutečné kliknutí přes AddMousePosEvent/AddMouseButtonEvent na keybind, přiřazení Mouse 4 a reset Escape. Ověřena také integrace do druhého CMake projektu s vlastní knihovnou ImGui a GETREKT_BUNDLED_IMGUI=OFF. To není záruka kompatibility s libovolným cizím hostem nebo jinou verzí.

## Vizuální kontrola migrace

`GetRektUIDemo.exe --capture` vytvoří složku qa s 11 BMP náhledy přes vlastní DX11 render target. Balíček obsahuje PNG kopie: page-0 až page-5 = hlavní stránky, page-6 = Advanced, page-7 = světlý režim, page-8 = Change log, page-9/10 = prázdný/naplněný spectator list. Vzhled, pevné rozměry a ovládání vycházejí z předchozí verze; jde o portování UI, nikoli změnu jeho funkcí. Rasterizace písma a drobné hranové pixely nové knihovny se mohou lišit.

Prázdný Spectator list zachovává pod hlavičkou prázdnou plochu o výšce jednoho řádku (celkem 72 px při šířce 224 px). Nezobrazuje žádný placeholder, avatar ani zprávu. Po přidání položek výška roste podle jejich počtu. Barvy stále přebírá z menu.
