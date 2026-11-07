# Prompt pro další chat

Přikládám ZIP **GetRekt-ImGui-Kit-Modern**. Chci s ním pracovat jako s opakovaně použitelným C++ UI modulem. Nejprve přečti README.md, CMakeLists.txt a include/getrekt/GetRektUI.hpp, pak prohlédni příklad. Nepředpokládej historii předchozího chatu.

## Co to je

Samostatné Dear ImGui rozhraní vyčleněné z GetRektLab 1.5. Zobrazuje menu, nastavení, klikací keybindy, náhled postavy, klubový changelog a spectator list. Všechny herně pojmenované položky jsou pouze lokální stav UI. Není to funkční cheat ani integrace do CS2; žádné čtení cizí paměti, injection, loader nebo anti-cheat bypass nejsou součástí modulu. Nová ukázka je čisté C++ Win32/DX11, bez LANO, C# a .NET.

## Struktura

- include/getrekt/GetRektUI.hpp — veřejný wrapper getrekt::UI, drží state, draw, focus/visibility/panic a keyEvent.
- include/getrekt/detail/VisualReplica.hpp — vlastní rozložení a lokální stav; vychází z finální verze 1.5.
- include/getrekt/detail/GetRektTheme.h — barvy, styl a společné prvky.
- include/getrekt/Win32Input.hpp — volitelný vstupní adaptér; hlavní modul neobsahuje Windows/DX11 API.
- vendor/imgui — oficiální Dear ImGui 1.93.0, bez lokálních patchů knihovny; oddělené backendy v vendor/imgui/backends.
- include/getrekt/detail/ReferenceColorPicker.hpp — původní vlastní picker zachovaný vzhledově i funkčně, přenesený mimo ImGui core.
- examples/win32_dx11.cpp — plně sestavitelná samostatná aplikace a příklad života textur.
- assets/preview_assets.h — volitelné statické RGBA náhledy z dodaného videa; nejsou to živé 3D modely.
- tests/smoke.cpp — test vykreslení UI bez rendereru a pravidel vstupu.

## Zachovej tyto požadavky

1. Text v LEGIT / Normal musí vždy respektovat nastavený Text color, nesmí mít zelenou barvu natvrdo. Akcent nadpisů může používat Theme color.
2. Normal a Advanced jsou odlišné; Normal má šest kategorií a nezávisle rolovatelné levé panely Aimbot / Custom settings / Non sticky aim a RCS, vpravo Triggerbot.
3. Kliknutí na hranatý popisek zachytí klávesu či tlačítko Mouse 1–5; Escape vymaže na key not set. Kliknutí na popisek nemá přepnout checkbox. Zachovej focus, repeat a rezervované klávesy.
4. Change log má pevné okno a scroll uvnitř, bez ručního resize.
5. Spectator list používá OPRAVENOU referenci: pozadí podle barev menu, pevná šířka 224 px, nahoře Spectator list. Každý řádek má avatar vlevo a vpravo nick, pod nickem 1st person nebo 3rd person. Žádné sloupce Name/Mode a žádný nick pod avatarem. Výška závisí na počtu dodaných položek; ruční resize je zakázán. Barvy nesmějí být pevně zadané: tělo = colors.config.windowBg, hlavička = accent, text = text, placeholder avataru = panelBg. Musí reagovat na nastavení barev menu i Light/Dark. Výchozí seznam je prázdný; host dodává data i textury.
6. ANTI-AIM nahrazuje původní RAGE; nevracej odstraněné AIM a Cams. VISUALS mění statický náhled podle checkboxů.
7. Zachovej čitelnost světlého režimu a izolaci stylu od ostatního UI aplikace.

## Jak integrovat

Host musí vlastnit ImGui kontext a backend. UI instanci drž dlouhodobě a volej draw() mezi NewFrame a Render na stejném vlákně. Na Windows volej Win32Input před backendem i aplikačními zkratkami. Na jiné platformě přelož události do kódů popsaných v README, ne do ImGuiKey. ImTextureID jsou zapůjčené, host musí zajistit životnost textur. Aktuální okna používají pevná ID, proto jedna instance na kontext.

Než integruješ, zjisti verzi ImGui a backend cílového projektu. Tento balíček JE JIŽ PORTOVANÝ na moderní Dear ImGui 1.93.0. Nevracej ImGui 1.51 ani starý kombinovaný backend. Při použití stejné existující verze vypni GETREKT_BUNDLED_IMGUI a připoj její CMake target k getrekt_ui. Nelinkuj dvě kopie knihovny. Pro jinou verzi proveď kompatibilitní kontrolu, nikoli slepé nahrazení hlavičky.

Životní cyklus: CreateContext, inicializace Win32 a DX11 backendu; každý snímek DX11_NewFrame → Win32_NewFrame → ImGui::NewFrame → menu.draw → ImGui::Render → DX11_RenderDrawData; na konci shutdown obou backendů a DestroyContext. Renderer jiné platformy je možný, ale nebyl otestován. Moderní backendy obsluhují dynamické fontové textury. Výchozí ImTextureID je číselný, pro DX11 použij (ImTextureID)(intptr_t)srv a pro prázdnou texturu 0.

Vstupní keyEvent nadále přijímá stabilní VK-kompatibilní kódy popsané v README, nikoli ImGuiKey. Win32Input je jejich adaptér; ImGui backend samostatně používá novou frontu vstupních událostí. Nepiš přímo do io.KeysDown nebo io.MouseDown. Vstup modulu musí proběhnout před aplikačními zkratkami. Zachovej setFocused a události key-up.

Debug testy migrace kontrolují ImGui assertions, skutečné kliknutí na badge, Mouse 4 a Escape, vzhledová data a zachování host stylu. QA náhledy jsou ve složce qa. Neoznačuj jiný cílový host za ověřený, dokud v něm neproběhne sestavení a test.


Profily jsou zatím pouze v paměti. Interní mapy mají klíče odvozené z ImGui ID; při připojování aplikace navrhni stabilní pojmenovaný model a případné ukládání odděleně. Online údaje a historický Change log jsou ilustrační. Neoznačuj tyto funkce jako skutečně propojené se Steamem/hrou.

Sestav přes CMake a spusť ctest. Pro samostatnou DX11 kontrolu spusť GetRektUIDemo.exe --smoke. Při práci popiš skutečně ověřenou kompatibilitu a omezení; netvrď, že projekt funguje v hostu, ve kterém nebyl otestován.

## Můj další úkol

[ZDE DOPLNÍM: cílový projekt a cestu, jeho ImGui verzi/backend a konkrétní požadovanou úpravu.]

Prázdný Spectator list zachovává pod hlavičkou prázdnou plochu o výšce jednoho řádku (celkem 72 px při šířce 224 px). Nezobrazuje žádný placeholder, avatar ani zprávu. Po přidání položek výška roste podle jejich počtu. Barvy stále přebírá z menu.
