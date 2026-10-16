# Pseudo-3D — roadmapa

Cel: Raptor widziany jak w grach z kamerą za statkiem (OutRun / Space Harrier / Mode 7). Logika gry (kolizje, spawn, ruch) zostaje 2D i nietknięta; zmienia się tylko kamera i sposób rysowania. Każdy etap kończy się czymś do obejrzenia i decyzją, czy idziemy dalej.

Inspiracja: [Lou's Pseudo 3D Page](https://www.extentofthejam.com/pseudo/).

## Co poszło nie tak w alpha
Warp gotowego ekranu 288×200: brak świata poza ekranem (clamp → smugi), sprite'y płaskie jak na planszy, brak kamery. Prawdziwe pseudo-3D potrzebuje: **świata** (tekstura podłogi szersza i dłuższa niż ekran), **rzutowania z kamery** i **billboardów**.

## Fakty z kodu (zbadane)
- Mapa: 150 rzędów × 9 kolumn × 32 px (288 px), wszystkie bitmapy kafli w cache po wczytaniu poziomu (`TILE_CacheLevel`, `src/tile.cpp:246`). Kafel = `GLB_GetItem(titems[row*9+col]) + 20` (32×32, 8-bit). `titems` jest żywe (zniszczone kafle). Nie wolno wołać `TILE_Display` (przesuwa scroll) ani `TILE_Put` (zapisuje do interp) — potrzebna czysta funkcja `TileWorld_Draw(r0, r1, dst, stride)`.
- Mapa jest „do tyłu": wiersz 0 = koniec poziomu, 149 = start; scroll = 1 px/tick (23,3 tick/s); `last_tile` zatrzymuje scroll. Poza 0..149 brak danych (niebo/horyzont).
- Obiekty (wrogowie, pociski, bonusy, gracz) są w pikselach ekranu (lewy górny róg sprite'a); brak współrzędnej wysokości. Kolejność rysowania = niejawna wysokość: tiles → ground shadows → ground enemies → sky shadows → ground anims → air enemies → shots → bonusy → mid-air anims → gracz → high anims → eshots → HUD (`src/rap.cpp:1075-1126`).
- Spawn wrogów: ~97–130 px nad ekranem (3 rzędy nad górnym narysowanym rzędem), nie 15–20 rzędów. Dalej istnieje tylko lista `csprite` (do podglądu).
- Cień: stały offset (+10, −20 px) przez `GFX_3DPoint` (`src/shadow.cpp`), niezależny od wysokości.
- Gracz: x 5..282 (środek 21..298), y 0..160, do 10 px/tick.
- Wyjście: 8-bit 320×200 → ARGB → tekstura; logiczny rozmiar 320×240 (aspect). HUD to osobny overlay (`I_SetOverlay`), `DrawShiftedPlayfield` trzeba wyłączyć w 3D. Strona webowa kopiuje canvas do rozmytych boków — działa z dowolnym wyjściem.
- Testy automatyczne: klawisz **D** w menu głównym uruchamia wbudowane demo w prawdziwej pętli `Do_Game` (bez imienia pilota). W buildzie webowym są syntetyczne klawisze (`sendKey` w `web/index.html`, brakuje `KeyD` w tabeli `KEYS`). Automatyzacja klawiatury w oknie Windows nie działa — rozwijamy i oceniamy na buildzie webowym w Chrome, exe dostarczamy użytkownikowi.

## Model kamery (docelowy)
Świat w pikselach: x ∈ [0, 288), głębokość `d` = odległość przed kamerą. Kamera na wysokości `Hc`, horyzont w wierszu `hor`, ogniskowa `f`. Dla wiersza wyjścia `r > hor`: `z = Hc·f/(r − hor)` (Lou: `Z = Y_world/(Y_screen − h/2)`), skala `s = f/z`; świat-x piksela `c`: `cam_x + (c − W/2)·z/f`; świat-y: `cam_y − (z − z0)·k`. Poza mapą w X: wypełnienie (mgła / powtórzenie krawędzi), poza końcem mapy: niebo.

Billboardy: obiekt z ekranowego `(x, y)` → `d = y_gracza − y` → `z = z0 + d·k` → `sx = W/2 + (x − cam_x)·f/z`, `sy = hor + (Hc − elev)·f/z`, skala `f/z`; rysowanie od najdalszego (painter). Kamera X podąża za `player_cx` (wygładzona). Mgła odległościowa przez tablicę cieniowania. Wyjście ARGB 640×400 (2×) na obszarze planszy; HUD na wierzchu.

## Etapy
- **E0 — Narzędzia testowe.** `KeyD` w tabeli `KEYS`; skrypt JS (Start → Enter/D → czekanie na `Module.raptorTicks` → zrzut Chrome); build WASM w WSL Debian + lokalny serwer; parametry strojenia w argumencie (`-p3d horizon=..,tilt=..`) lub INI. *Kryterium:* powtarzalnie dochodzę do rozgrywki demo i robię zrzut bez ręki. ⬜
- **E1 — Sama podłoga (bez sprite'ów).** `TileWorld_Draw` → bufor świata 8-bit; `P3D_RenderGround` z tablicami per wiersz, próbkowanie z bufora świata, ARGB 640×400 przez paletę (getter w `i_video`), wypełnienie boków, niebo/horyzont; `I_SetPseudo3D` w `i_video.cpp`, wyłączenie `DrawShiftedPlayfield`. *Kryterium:* ostra podłoga z horyzontem bez smug (mocna perspektywa). Dostawa: pre-release exe + podgląd webowy lokalnie. ⬜
- **E2 — Warstwy i billboardy.** `INTERP_Layer(1)` przed `ENEMY_DisplaySky` (`src/rap.cpp`); `layer` w `IOP`; ground-ops (kafle, gshadow, wrogowie naziemni, anim ground) → dekale na podłodze; air-ops `SPRITE` → billboardy (dekoder sprite'a z cache po `pic` + maska, skalowanie, sortowanie od tyłu); cień jako dekal z offsetem; prymitywy (`LINE/COLORBOX/VLINE/FLAME*`) w osobnym podetapie. *Kryterium:* statek i wrogowie stoją pionowo i rosną przy zbliżaniu. ⬜
- **E3 — Kamera, horyzont, atmosfera.** Kamera podąża za statkiem w X (wygładzenie), lekki roll opcjonalnie, niebo (gradient + paralaksa), mgła odległościowa, parametry horyzontu. ⬜
- **E4 — Czytelność gry.** Kolizje zostają 2D; sprawdzić, czy pociski i unikanie są czytelne (uwaga z recenzji Axelay: trudno ocenić ominięcie); cień pod statkiem, ewentualna linia celowania, presety pochylenia (Niskie / Średnie / Wysokie). ⬜
- **E5 — Zasięg i pop-in.** Wrogowie pojawiają się ~100–130 px nad ekranem → maskowanie mgłą/horyzontem; opcjonalnie podgląd obiektów naziemnych z `csprite`; rysowanie kafli do 20 rzędów w przód. ⬜
- **E6 — Opcje i UI.** `-pseudo3d`, wpis w SETUP (Additional Features → View: Flat / Pseudo-3D + preset), selektor na stronie webowej, zapis w INI; tryb Classic bez 3D. ⬜
- **E7 — Wydajność i jakość.** Cel < 4 ms/klatkę, tablice per wiersz, filtrowanie w pionie, tryb „chunky” 320×200 vs 640×400, pomiar FPS w WASM. ⬜
- **E8 — Wydanie i dokumentacja.** Pre-release exe po każdym ważnym etapie, README, `notes/lekcja-06`, scalanie do `web` (produkcja) dopiero po osobnej zgodzie. ⬜

## Ryzyka
Pop-in wrogów (spawn tuż nad ekranem) • czytelność trafień przy dużym pochyleniu • hitboxy 2D vs wygląd 3D • HUD/overlay, shake ekranu, fade palety • tryb demo / determinizm • koszt CPU w WASM • tryb Classic nie ma zapisanych operacji, więc 3D go wyłącza.

## Weryfikacja (każdy etap)
Build WASM + Chrome (demo przez `D`), zrzuty flat vs 3D tego samego ticka, test determinizmu demo (`-pseudo3d 0` vs `1`, ten sam przebieg), pomiar czasu klatki, smoke test exe w Windows, dopiero potem pre-release.

## Zasady pracy
Po każdym etapie zatrzymuję się i pokazuję wynik. Commity tylko na `pseudo-3d`. Nic na `web` bez zgody. Wydania wyłącznie jako pre-release. Status etapów (⬜ / 🟨 / ✅) aktualizowany na bieżąco.
