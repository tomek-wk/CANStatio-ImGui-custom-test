# Postęp prac

## Stan na 2026-09-24

Normalny branch rozwojowy:

```text
main
```

Repozytorium jest niezależnym eksperymentem Dear ImGui z własną implementacją wykresu. Skopiowany wcześniejszy prototyp był wyłącznie bazą migracyjną; rozwiązania techniczne poprzedniego renderera nie są częścią aktualnej architektury.

## Specyfikacja

**Status: kompletna bazowa specyfikacja pierwszego eksperymentu.**

`docs/PROJECT_SPEC.md` definiuje model danych, rendering, osie, nawigację, active/visibility, Values, cursory, markery, input priority, presety, exclusive fullscreen i kryterium sukcesu.

## Implementacja

### C0–C7

Bazowa implementacja custom chart jest kompletna funkcjonalnie:

- app-owned plot area i transformacje data/screen;
- własne osie X/Y, ticki i grid;
- niezależny Y każdej serii;
- pan/zoom X i Y;
- Fit X / Fit Y;
- active series i visibility;
- RMB hit-test serii;
- Halo / Outline;
- Values i crosshair;
- cursory A/B;
- markery;
- jawna `InputPolicy`;
- pełny zestaw presetów od Small do Stress Raw;
- raw rendering bez LOD/downsamplingu.

Pomiary Reference / Stress Raw i decyzja o potrzebie LOD nadal pozostają do wykonania.

### Tryb okna

`F11` przełącza docelowy **exclusive fullscreen** i przy wyjściu przywraca poprzednią geometrię zwykłego okna.

Tymczasowe tryby diagnostyczne F8/F9/F10 zostały usunięte po zakończeniu badania artefaktów. Nie należały do specyfikacji produktu.

### Backend renderujący

Domyślny backend pozostaje bez zmian:

```text
OpenGL 3.3 / GLFW / WGL
```

Uruchomienie:

```text
run
run --backend=opengl
```

Na Windows zachowany jest opcjonalny backend DirectX 11:

```text
run --backend=dx11
```

DX11 używa:

```text
IDXGIFactory2::CreateSwapChainForHwnd
DXGI_SWAP_EFFECT_FLIP_DISCARD
BufferCount = 2
```

`run.bat` przekazuje argumenty CLI do programu.

## Presety

- Small / Sanity — 3 × 1 000, 100 ms;
- Reference — 60 × 36 000, 100 ms;
- Overlap — 60 × 36 000;
- Mixed Scale — 60 × 36 000;
- Spikes / Noise — 60 × 36 000;
- Long Time — 10 × 108 000, 100 ms;
- Stress Raw — 60 × 360 000, 10 ms.

Renderer nadal działa raw, bez LOD/downsamplingu. Widoczny zakres próbek jest ograniczany do viewportu X z małym zapasem.

## Reset i preset switching

Reset:

- Fit X do pełnego datasetu;
- wszystkie serie visible;
- brak active;
- Fit Y każdej serii;
- cursory A/B hidden;
- markers cleared;
- marker counter = 0.

## Testy

Testowalne komponenty obejmują:

- `ChartMath` — transforms, pan/zoom, nice/time ticks, point-to-segment;
- `InputPolicy` — priorytety modifierów i brak fallbacków;
- `MarkerModel` — ID, clamp, move, remove, reset;
- `DataGenerator` — Small specification, determinism i nazwy presetów;
- zachowane `Dataset`, `Series`, `ValuesModel`, `CursorModel`;
- GUI smoke/integration — startup, okna, Custom Legend, highlight, plot geometry/mouse-time i Fit X.

### Aktualna automatyczna weryfikacja oczyszczonego kodu

Kod po usunięciu F8/F9/F10 został zbudowany i przetestowany na target Windows jako commit:

```text
commit: a244fc0
configure/build: PASS
functional: 41/41 PASS
GUI integration: 6/6 PASS
```

JUnit:

```text
build/test-results/functional-tests.xml
build/test-results/imgui-tests.xml
```

GUI suite w tej weryfikacji pracował na domyślnym OpenGL i zarejestrował:

```text
OpenGL vendor: Intel
OpenGL renderer: Intel(R) HD Graphics 530
OpenGL version: 3.3.0 - Build 30.0.101.1692
```

## Znany problem prezentacji Windows/OpenGL

Na testowym Windows 10 z Intel HD Graphics 530 i sterownikiem `30.0.101.1692` w dużym, szczególnie zmaksymalizowanym zwykłym oknie OpenGL mogą pojawiać się bardzo krótkie losowe kolorowe błyski/prostokąty.

Wyniki diagnostyki:

```text
OpenGL, maximized                  -> migotanie występuje
ImPlot/OpenGL                      -> podobny problem, zwykle mniej widoczny
AntiAliasedLinesUseTex = false     -> wyraźna poprawa, ale nie pełne usunięcie
MPO wyłączone                      -> problem nadal występuje
DX11 legacy DISCARD                -> problem nadal występuje
DX11 FLIP_DISCARD                  -> brak zaobserwowanego migotania
exclusive fullscreen               -> brak zaobserwowanego migotania
```

Podczas wcześniejszej diagnostyki wielkości/rodzaju okna obserwowano również:

```text
maximized decorated      -> dużo błysków
large decorated workarea -> mniej błysków
undecorated workarea     -> bardzo rzadkie błyski
borderless full monitor  -> brak zaobserwowanych błysków
exclusive fullscreen     -> brak zaobserwowanych błysków
```

Wniosek roboczy: problem jest związany ze ścieżką prezentacji/kompozycji dużego okna Windows, a nie z samą geometrią custom chartu. Natywne OpenGL/WGL nie daje aplikacji bezpośredniego odpowiednika ustawienia `DXGI_SWAP_EFFECT_FLIP_DISCARD`.

Decyzja projektowa:

- **OpenGL pozostaje domyślny i nie jest dalej przebudowywany w ramach tej diagnostyki**;
- `AntiAliasedLinesUseTex = false` pozostaje włączone jako proste ograniczenie widoczności artefaktu;
- `--backend=dx11` pozostaje dostępny jako alternatywny backend Windows;
- DX11 używa `FLIP_DISCARD`, ponieważ w ręcznym teście zmaksymalizowanego okna nie zaobserwowano na nim migotania;
- dalsze eksperymenty ANGLE/WGL/D3D interop nie są obecnie planowane.

Badanie artefaktów jest zakończone.

## Co pozostaje niezweryfikowane

- pełna manualna ocena ergonomii rzeczywistych gestów X/Y, RMB, cursorów i markerów;
- performance Reference;
- performance Stress Raw;
- decyzja o potrzebie LOD/downsamplingu.

Aktualny GUI suite jest smoke/integration suite i nie obejmuje jeszcze end-to-end wszystkich gestów zaimplementowanych w C2–C6. Matematyka i polityka inputu są pokryte testami functional.

## Następny krok

Prace wracają do głównego celu eksperymentu: manualnej walidacji UX oraz pomiarów Reference / Stress Raw, a następnie ewentualnego rozszerzenia GUI regression o najważniejsze rzeczywiste gesty:

```text
plain drag / wheel X
Alt drag / wheel Y
RMB selection
cursor set / drag / hide
marker create / drag / remove
modifier conflicts
```

Dopiero wyniki Reference / Stress Raw powinny zdecydować, czy potrzebny jest LOD/downsampling lub dalsza optymalizacja.
