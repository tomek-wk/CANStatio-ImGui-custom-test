# CANStatio ImGui Custom Chart Test — instrukcje dla AI

## Cel repozytorium

To repozytorium jest samodzielnym eksperymentem służącym do oceny Dear ImGui z własną implementacją wykresu jako potencjalnego frontendu wykresów dla CANstatio LogViewer.

Nie jest to produkcyjny LogViewer. Nie należy automatycznie synchronizować jego architektury ani UX z głównym repozytorium CANStatio.

## Nadrzędna granica repozytorium

Pracuj wyłącznie w `CANStatio-ImGui-custom-test`.

Nie zapisuj, nie zmieniaj i nie usuwaj niczego w innych repozytoriach. Repozytorium Test Agenta może być używane wyłącznie jako kanał komunikacji ze środowiskiem testowym; jego zawartość nie jest częścią zakresu zmian.

## Co przeczytać przed pracą

1. `AGENTS.md`
2. `docs/PROJECT_SPEC.md`
3. `docs/DECISIONS.md`
4. `docs/PROGRESS.md`
5. `docs/AUTOMATED_TESTS.md` — dla testów/weryfikacji
6. odpowiednie pliki źródłowe

## Hierarchia źródeł prawdy

1. bieżące jawne polecenie użytkownika;
2. `docs/AUTOMATED_TESTS.md` — tylko w zakresie testów;
3. `docs/PROJECT_SPEC.md`;
4. `docs/DECISIONS.md`;
5. `docs/PROGRESS.md`;
6. bieżąca implementacja.

Nie dopasowuj po cichu wymagań do kodu. Jeśli implementacja i dokumentacja się rozjechały, ustal która warstwa jest nieaktualna i popraw ją jawnie.

## Model pracy z branchami

Normalny rozwój odbywa się liniowo na `main`. Osobny branch ma sens tylko dla rzeczywiście izolowanego dużego eksperymentu architektonicznego/toolchainowego/infrastrukturalnego.

## Zasady pracy

Przy większym zadaniu:

1. sprawdź aktualny `main`;
2. utrzymuj zgodność ze specyfikacją, nie z historią starego renderera;
3. nie dodawaj abstrakcji ani zależności bez realnej potrzeby;
4. współdziel jedną matematykę data/screen między renderingiem i narzędziami;
5. po zmianach uruchom build/testy tylko w rzeczywiście dostępnym środowisku;
6. nie deklaruj PASS bez rzeczywistego uruchomienia;
7. wygląd, ergonomię i performance traktuj oddzielnie od automatycznego PASS/FAIL;
8. po istotnej zmianie zaktualizuj `docs/PROGRESS.md`;
9. decyzje trwałe zapisuj w `docs/DECISIONS.md`, zmiany wymagań także w `docs/PROJECT_SPEC.md`.

## Granice eksperymentu

Bez osobnej decyzji nie dodawaj:

- parsera CSV/otwierania plików;
- DataCore ani modelu projektu CANStatio;
- Overview/minimapy;
- irregular sampling ani osobnych timestampów serii;
- NaN/gaps i step/digital;
- LOD/downsamplingu przed pomiarem Reference/Stress Raw;
- integracji z głównym CANStatio;
- dodatkowej biblioteki wykresowej;
- zewnętrznego OS-level GUI automation, jeśli Dear ImGui Test Engine wystarcza;
- nowego frameworka functional, jeśli GoogleTest + CTest wystarcza.

## Stack

- C++20
- Windows first
- GCC / MinGW-w64 (MSYS2 UCRT64)
- CMake + Ninja
- GLFW
- OpenGL 3.3 Core jako domyślny backend
- opcjonalny DirectX 11 na Windows przez `--backend=dx11`, z DXGI flip model
- Dear ImGui `v1.92.9b`
- Dear ImGui Test Engine `508a8fc8dacac2f346d353fed31b9bc90ed29adc`
- GoogleTest + CTest

## Aktualna architektura

Pierwszy pełny wariant custom chart jest zaimplementowany.

Warstwa aplikacji jest właścicielem:

- plot area i stabilnego layoutu;
- data/screen transforms (`ChartMath`);
- osi, ticków, etykiet i grida;
- raw renderingu serii i clippingu;
- hit-testu serii i overlayów;
- pan/zoom X;
- pan/zoom Y active visible series;
- active highlighting;
- crosshair;
- cursorów A/B (`CursorModel`);
- markerów (`MarkerModel`);
- priorytetów inputu (`InputPolicy`).

Nie istnieje wspólna sztuczna przestrzeń Y. Każda seria mapuje własne `viewMin/viewMax` bezpośrednio do tego samego plot rectangle.

## Najważniejsze zachowanie

- regularny wspólny X;
- niezależny Y każdej serii;
- fixed plot geometry przy zmianie active;
- plain LMB drag / wheel = pan/zoom X;
- `Alt + drag/wheel` = pan/zoom Y active visible series;
- RMB = nearest-series selection;
- `Shift + LMB` / `Ctrl + LMB` = set A/B;
- plain drag cursor/marker = move;
- `Ctrl + RMB` cursor/marker = hide/remove;
- `Alt + click` = marker, `Alt + drag` po threshold = Y pan;
- modifier gestures są wyłączne i nie fallbackują do X/selection;
- Values/crosshair/cursors/markers korzystają ze wspólnego systemu współrzędnych.

Pełna semantyka znajduje się w `docs/PROJECT_SPEC.md`.

## Strategia testów

1. **functional C++** — `ChartMath`, modele, `InputPolicy`, dataset/Values i edge-case'y;
2. **GUI integration** — Dear ImGui Test Engine;
3. **manual** — wygląd, ergonomia i performance.

Docelowy Windows PC jest preferowanym środowiskiem weryfikacji. Test Agent jest dostępny i służy wyłącznie jako kanał komunikacji z tym środowiskiem; nie modyfikuj jego repozytorium ani konfiguracji.

GitHub-hosted Windows pozostaje ręcznym `workflow_dispatch` fallbackiem build + functional.

## Aktualny stan weryfikacji

Pełna implementacja custom chart została zbudowana i zweryfikowana na docelowym Windows PC. Po oczyszczeniu tymczasowych trybów diagnostycznych F8/F9/F10 potwierdzono:

```text
configure/build: PASS
functional: 41/41 PASS
GUI integration: 6/6 PASS
```

OpenGL pozostaje domyślnym backendem. Znany problem krótkich artefaktów prezentacji w dużym/maximized oknie na testowej konfiguracji Intel HD Graphics 530 jest opisany w `README.md`, `docs/DECISIONS.md` i `docs/PROGRESS.md`. Opcjonalny `--backend=dx11` używa `DXGI_SWAP_EFFECT_FLIP_DISCARD` i w ręcznym teście nie wykazał migotania.

Nie traktuj historycznych wyników skopiowanego prototypu jako wyniku custom renderera.

Najbliższa praca to manualna walidacja UX, rozszerzenie najważniejszych GUI regression oraz pomiary Reference / Stress Raw przed decyzją o LOD/downsamplingu.